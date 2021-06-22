// clang-format off
#include "nain4.hh"
#include "g4-mandatory.hh"
#include "random/random.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/samples.hh"
#include "geometries/sipm.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>
#include <G4GenericMessenger.hh>

#include <G4OpticalPhoton.hh>
#include <G4Gamma.hh>
#include <Randomize.hh>

#include <memory>
#include <ostream>
#include <string>
#include <variant>

using std::make_unique;
using std::unique_ptr;

// ----- map/set helpers --------------------------------------------------------------------
template<class M, class K>
bool contains(M const& map, K const& key) { return map.find(key) != end(map); }

#include <set>
using times_set = std::multiset<double>;

// ----- Choices which can be made in configuration files -----------------------------------
struct abracadabra_messenger {
  abracadabra_messenger() : messenger{new G4GenericMessenger{this, "/abracadabra/", "It's maaaaagic!"}} {
    // TODO units, ranges etc.
    messenger -> DeclareProperty("event-number-offset", offset, "Starting value for event ids");
    messenger -> DeclareProperty("outfile"   , outfile   , "file to which hdf5 tables well be written");
    messenger -> DeclareProperty("geometry"  , geometry  ,  "Geometry to be instantiated");
    messenger -> DeclareProperty("detector"  , detector  ,  "Detector to be instantiated");
    messenger -> DeclareProperty("phantom"   , phantom   ,  "Phantom to be used");
    messenger -> DeclareProperty("spin_view" , spin      , "Spin geometry view");
    messenger -> DeclareProperty("spin_speed", spin_speed, "Spin geometry speed");
    messenger -> DeclareProperty("xenon_thickness", xenon_thickness, "Thickness of LXe layer");
    messenger -> DeclareProperty("cylinder_length", cylinder_length, "Length of cylinder");
    messenger -> DeclareProperty("cylinder_radius", cylinder_radius, "Radius of cylinder");
    messenger -> DeclareProperty("imas_version"   , imas_version   , "Version of detector design");
    messenger -> DeclareProperty("clear-pre-lxe"  , vac_pre_lxe    , "Remove obstacles before LXe");
  }
  size_t offset = 0;
  G4String outfile    = "default-out.h5";
  G4String geometry   = "both";
  G4String detector   = "imas";
  G4String phantom    = "nema_7";
  bool     spin       = true;
  G4int    spin_speed = 10;
  G4double xenon_thickness =  40 * mm;
  G4double cylinder_length =  15 * mm;
  G4double cylinder_radius = 200 * mm;
  unsigned imas_version = 1;
  bool vac_pre_lxe = false;
private:
  unique_ptr<G4GenericMessenger> messenger;
};
// --------------------------------------------------------------------------------------------
struct generator_messenger : G4UImessenger {
  generator_messenger(std::map<G4String, n4::generator::function>& choices)
    : dir{new G4UIdirectory     ("/generator/")}
    , cmd{new G4UIcmdWithAString("/generator/choose", this)}
    , generate{[] (auto) { throw "No generator has been chosen"; }}
    , choices{choices}
  {
    auto default_ = "phantom";
    dir -> SetGuidance("TODO blah blah blah");
    cmd -> SetGuidance("choice of primary generator");
    cmd -> SetParameterName("choose", /*omittable*/ false);
    cmd -> SetDefaultValue(default_);
    cmd -> SetCandidates(""); // TODO get these from the generator map
    cmd -> AvailableForStates(G4State_PreInit, G4State_Idle);
    generate = choices[default_];
  }

  void SetNewValue(G4UIcommand* command, G4String choice) {
    if (command == cmd.get()) {
      // TODO use proper exceptions
      if (!contains(choices, choice)) { throw "Unrecoginzed generator " + choice; }
      generate = choices[choice];
    }
  }
  n4::generator::function generator() { return generate; }
private:
  unique_ptr<G4UIdirectory> dir;
  unique_ptr<G4UIcmdWithAString> cmd;
  n4::generator::function generate;
  std::map<G4String, n4::generator::function>& choices;
};

// =============================================================================================
// ----- UI: Abstract class with two concrete implementations: interactive and batch -----------
void stop_if_failed(G4int status) { if (status != 0) { throw "up"; } }

struct UI {
  static UI* make(int argc, char** argv, abracadabra_messenger&); // Polymorphic constructor
  UI(char** argv, abracadabra_messenger& messenger) : messenger{messenger} {
      G4String model_filename = argv[1];
      stop_if_failed(ui_manager -> ApplyCommand("/control/execute " + model_filename));
  }
  virtual void run() = 0;
  //private:
  G4UImanager* ui_manager = G4UImanager::GetUIpointer(); // G4 manages lifetime
  abracadabra_messenger& messenger;
};
// ----------------------------------------------------------------------------------------------
struct UI_interactive : public UI {
  UI_interactive(int argc, char** argv, abracadabra_messenger& messenger) : UI{argv, messenger} {
      ui =          make_unique<G4UIExecutive>(argc, argv);
      vis_manager = make_unique<G4VisExecutive>();
  }
  void run() {
    // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
    // G4VisManager* visManager = new G4VisExecutive{"Quiet"};
    vis_manager -> Initialize();
    stop_if_failed(ui_manager -> ApplyCommand("/control/execute macs/init_vis.mac"));
    spin();
    ui -> SessionStart();
  }
  void spin();
  unique_ptr<G4UIExecutive>           ui{nullptr};
  unique_ptr<G4VisExecutive> vis_manager{nullptr};
};
// ----------------------------------------------------------------------------------------------
struct UI_batch : public UI {
  UI_batch(int, char** argv, abracadabra_messenger& messenger) : UI{argv, messenger}, run_macro_filename{argv[2]} {}
  void run() {
    stop_if_failed(ui_manager -> ApplyCommand("/control/execute " + run_macro_filename));
  }
  G4String run_macro_filename;
};

UI* UI::make(int argc, char** argv, abracadabra_messenger& messenger) {
  if (argc  < 2) {
    std::cerr << "A model macro file must be provided" << std::endl;
    throw "A model macro file must be provided";
  }
  if (argc == 2) { return new UI_interactive(argc, argv, messenger); }
  else           { return new UI_batch      (argc, argv, messenger); }
}

// ----- Polymorphic access to phantoms without subclassing
struct phantom_t {
  phantom_t(n4::generator::function generate, n4::geometry::construct_fn geometry)
    : generate{generate}
    , geometry{geometry}
  {}
  const n4::generator::function    generate;
  const n4::geometry::construct_fn geometry;
};

auto combine_geometries(G4VPhysicalVolume* phantom, G4VPhysicalVolume* detector) {
  auto detector_envelope = detector -> GetLogicalVolume();
  auto phantom_envelope  =  phantom -> GetLogicalVolume();
  auto phantom_contents = phantom_envelope -> GetDaughter(0) -> GetLogicalVolume(); // TODO Can we avoid this?

  // Check whether phantom envelope fits inside detector envelope, with margin.
  auto& pbox = dynamic_cast<G4Box&>(* phantom_envelope -> GetSolid());
  auto& dbox = dynamic_cast<G4Box&>(*detector_envelope -> GetSolid());
  auto expand = false;
  auto make_space = [&expand](auto p, auto d) {
    auto space_needed = std::max(p * 1.1, d);
    if (space_needed > d) { expand = true; }
    return space_needed;
  };
  auto x = make_space(pbox.GetXHalfLength(), dbox.GetXHalfLength());
  auto y = make_space(pbox.GetYHalfLength(), dbox.GetYHalfLength());
  auto z = make_space(pbox.GetZHalfLength(), dbox.GetZHalfLength());
  // Expand detector envelope if needed
  if (expand) {
    auto new_box = new G4Box(detector_envelope->GetName(), x, y, z);
    detector_envelope -> SetSolid(new_box);
  }

  n4::place(phantom_contents).in(detector_envelope).now();
  return detector;
};

// ============================== MAIN =======================================================
int main(int argc, char** argv) {

  abracadabra_messenger messenger;
  auto current_event = [&]() { return n4::event_number() + messenger.offset; };

  // ----- collecting arrival times of optical photons in sensors ----------------------------
  std::map<size_t, times_set> times;

  auto add_to_waveforms = [&times](auto sensor_id, auto time) {
    if (!contains(times, sensor_id)) {
      times.emplace(sensor_id, times_set{});
    }
    auto& [_, set] = *times.find(sensor_id);
    set.insert(time);
  };

  // ----- Where to write output from sensitive detectors ------------------------------------
  unique_ptr<hdf5_io> writer;
  auto  open_writer = [&writer, &messenger]() { writer.reset(new hdf5_io{messenger.outfile});};

  // ----- Extract sensor positions from geometry and write to hdf5 --------------------------
  auto write_sensor_database = [&writer, &open_writer](auto geometry) {
    open_writer();
    for(auto* vol: geometry) {
       auto name = vol -> GetName();
      if (name.rfind("Hamamatsu_Blue", 0) == 0) { // starts with
        auto p = vol -> GetTranslation();
        auto id = vol -> GetCopyNo();
        writer -> write_sensor_xyz(id, p.x(), p.y(), p.z());
      }
    }
    return geometry;
  };

  // ----- Sensitive detector ----------------------------------------------------------------
  n4::sensitive_detector::process_hits_fn store_hits = [&add_to_waveforms](G4Step* step) {
    static auto optical_photon = G4OpticalPhoton::Definition();

    auto track    = step -> GetTrack();
    auto particle = track -> GetParticleDefinition();
    auto name     = particle -> GetParticleName();

    if (particle == optical_photon) {
      auto time = track -> GetGlobalTime();
      auto sensor_id = step -> GetPreStepPoint() -> GetTouchable() -> GetCopyNumber(1);
      add_to_waveforms(sensor_id, time);
      return true;
    }

    return false; // Still not *entirely* sure about the return value meaning
  };

  n4::sensitive_detector::end_of_event_fn write_hits = [&](auto) {
    size_t event_id = current_event();

    // std::cout << event_id << std::endl;
    // std::cout << " event  parent  id            x    y    z     r     moved    preKE pstKE   deposited" << std::endl;

    for (auto& [sensor_id, ts] : times) {
      auto start = *cbegin(ts);
      // std::vector<double> t{};
      // TODO reserve space once good size is known from statistics
      // std::copy_if(cbegin(ts), cend(ts), back_inserter(t),
      //              [start](auto t) { return t < start + 100 * ps; });
      // writer -> write_waveform(event_id, sensor_id, t);
      // writer -> write_total_charge(event_id, sensor_id, ts.size());
      if (sensor_id) {writer -> write_q_t0(event_id, sensor_id, ts.size(), start);}
    }
    times.clear();
  };

  // pick one:
  auto sd = new n4::sensitive_detector{"Writing_detector", store_hits, write_hits};
  //auto sd = nullptr; // If you only want to visualize geometry

  // ----- Available phantoms -----------------------------------------------------------
  auto nema_7 = []() {
    // TODO: the central cylinder is missing (7.3.3 c)
    return build_nema_7_phantom{}
      .activity(1)
      .length(180*mm)
      .inner_radius(57.2*mm)
      .outer_radius(77.0*mm)
      .sphereD(10*mm / 2, 4)
      .sphereD(13*mm / 2, 4)
      .sphereD(17*mm / 2, 4)
      .sphereD(22*mm / 2, 4)
      .sphereD(28*mm / 2, 4)
      .sphereD(37*mm / 2, 4)
      .build();
  };

  auto nema_3 = [&messenger]() {
    auto fov_length = messenger.cylinder_length * mm;
    return nema_3_phantom{fov_length};
  };

  // ----- Choice of phantom -------------------------------------------------------------
  // Can choose phantom in macros with `/abracadabra/phantom <choice>`
  // The nema_3 phantom's length is determined by `/abracadabra/cylinder_length` in mm

  using polymorphic_phantom = std::variant<nema_3_phantom, nema_7_phantom>;

  // A variable containing the phantom is needed early on, because it is
  // captured by various lambdas. Need to construct the variant with type that
  // satisfies the common interface, so std::monostate won't do. The specific
  // value will be overridden when config file is read.
  polymorphic_phantom phantom = nema_3();

  // Dispatch to the chosen phantom's `geometry` and `generate_primaries`
  // methods. This imposes a static Duck Type interface on the phantom types in
  // `polymorphic_phantom`.
  auto phantom_geometry = [&phantom](          ) { return std::visit([     ](auto& ph) { return ph.geometry          (     ); }, phantom); };
  auto phantom_generate = [&phantom](auto event) { return std::visit([event](auto& ph) { return ph.generate_primaries(event); }, phantom); };

  // Choose phantom in config file via `/abracadabra/phantom`
  auto set_phantom = [&](G4String p) {
    p == "nema_7" ? phantom = nema_7() :
    p == "nema_3" ? phantom = nema_3() :
    throw "Unrecoginzed phantom " + p;
  };

  // ----- Available detector geometries -------------------------------------------------
  // Can choose detector in macros with `/abracadabra/detector <choice>`
  auto detector = [&, &d = messenger.detector]() -> G4VPhysicalVolume* {
    auto dr_LXe  = messenger.xenon_thickness * mm;
    auto length  = messenger.cylinder_length * mm;
    auto radius  = messenger.cylinder_radius * mm;
    auto version = messenger.imas_version;
    auto clear   = messenger.vac_pre_lxe;
    return
      d == "cylinder"  ? cylinder_lined_with_hamamatsus(length, radius, dr_LXe, sd) :
      d == "imas"      ? imas_demonstrator(sd, length, version, clear)     :
      d == "square"    ? square_array_of_sipms(sd)                         :
      d == "hamamatsu" ? nain4::place(sipm_hamamatsu_blue(true, sd)).now() :
      throw "Unrecoginzed detector " + d;
  };
  // ----- Should the geometry contain phantom only / detector only / both
  // Can choose geometry in macros with `/abracadabra/geometry <choice>`
  auto geometry = [&, &g = messenger.geometry]() -> G4VPhysicalVolume* {
    return
      g == "detector" ? detector()         :
      g == "phantom"  ? phantom_geometry() :
      g == "both"     ? combine_geometries(phantom_geometry(), detector()) :
      throw "Unrecoginzed geometry " + g;
  };

  // ----- A choice of generators ---------------------------------------------------------
  // Can choose generator in macros with `/abracadabra/generator <choice>`
  std::map<G4String, n4::generator::function> generators = {
    {"origin"      , [ ](auto event) { generate_back_to_back_511_keV_gammas(event, {}, 0); }},
    {"phantom"     , [&](auto event) { phantom_generate(event); }},
    {"quarter_ring", [ ](auto event) {
      // Lots of asymmetry to help verify orientation
      auto r = 250 * mm;
      auto phi = uniform(0, CLHEP::pi / 2);
      auto x = r * cos(phi);
      auto y = r * sin(phi);
      auto z = 100  * mm;
      generate_back_to_back_511_keV_gammas(event, {x,y,z}, 0);
    }}
  };

  generator_messenger generator_messenger{generators};

  UI* ui = UI::make(argc, argv, messenger);
  set_phantom(messenger.phantom);

  // ----- Identifying vertices in LXe ----------------------------------------------------
  auto transp = [](auto name) { return name == "Transportation" ? "---->" : name; };

#define DBG(stuff) std::cout << stuff << std::endl;

  n4::stepping_action::action_t xxxstep = [&](auto step) {
    auto pst_pt = step -> GetPostStepPoint();
    auto process = pst_pt -> GetProcessDefinedStep();
    auto name = transp(process -> GetProcessName());
    auto track = step -> GetTrack();
    auto particle = track -> GetParticleDefinition();
    if (particle == G4Gamma::Definition()) {
      auto pre_pt = step -> GetPreStepPoint();
      auto pre_proc = pre_pt -> GetProcessDefinedStep();
      auto pre_name = transp(pre_proc ? pre_proc -> GetProcessName() : "  -  ");
      if (!pre_proc) { DBG(' ') }
      auto id = track -> GetTrackID();
      auto parent = track -> GetParentID();
      auto p = pst_pt -> GetPosition();
      auto x = p.x(); auto y = p.y(); auto z = p.z();
      auto r = sqrt(x*x + y*y);
      auto moved = step -> GetDeltaPosition().mag();
      auto dep_E = step -> GetTotalEnergyDeposit() / keV;
      auto volume_name = pre_pt -> GetPhysicalVolume() -> GetName();
      auto dynamic_particle = track -> GetDynamicParticle();
      auto dyn_TE = dynamic_particle -> GetTotalEnergy() / keV;
      auto dyn_KE = dynamic_particle -> GetKineticEnergy() / keV;
      auto pre_KE = pre_pt -> GetKineticEnergy() / keV;
      auto pst_KE = pst_pt -> GetKineticEnergy() / keV;
      auto pst_TE = pst_pt -> GetTotalEnergy() / keV;
      assert(pst_TE == pst_KE);
      assert(dyn_TE == dyn_KE);
      assert(dyn_TE == pst_TE);
      auto event_id = current_event();
      if (name == "---->") return;

      using std::setw;
      std::cout << std::setprecision(1) << std::fixed;
      DBG(setw(7) << event_id
          << setw( 5) << parent << ' '
          << setw( 5) << id
          << setw( 6) << name
          << "  (" << std::setw(5) << (int)x << std::setw(5) << (int)y << std::setw(5) << (int)z << " :" << std::setw(4) << (int)r << ") "
          << setw(7) << moved << "   "
          << setw(6) << pre_KE
          << setw(6) << pst_KE
          << setw(6) << dep_E
          << setw(20) << volume_name << ' '
          //<< particle -> GetParticleName()
          );
      auto t = 666;
      writer -> write_vertex(event_id, id, parent, x, y, z, t, moved, pre_KE, pst_KE, dep_E, name[0], volume_name);
    }
  };

  n4::event_action::action_t show_primary_generator = [&](auto event) {
    using std::setw;
    auto event_id = current_event();
    auto vertex = event -> GetPrimaryVertex();
    auto pos = vertex -> GetPosition();
    auto mom = vertex -> GetPrimary() -> GetMomentum();
    auto [ x, y, z] = std::make_tuple(pos.x(), pos.y(), pos.z());
    auto [px,py,pz] = std::make_tuple(mom.x(), mom.y(), mom.z());
    std::cout << setw(7) << event_id
              << setw(6) <<  x << setw(6) <<  y << setw(6) <<  z << " "
              << setw(6) << px << setw(6) << py << setw(6) << pz << " "
              << std::endl;
    writer -> write_primary(event_id, x,y,z, px,py,pz);
  };
  // ===== Mandatory G4 initializations ===================================================

  // Construct the default run manager
  auto run_manager = unique_ptr<G4RunManager> {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial)};

  // ----- Geometry (run_manager takes ownership) -----------------------------------------
  run_manager -> SetUserInitialization(new n4::geometry{[&write_sensor_database, geometry]() -> G4VPhysicalVolume* {
    return write_sensor_database(geometry());
  }});
  // ----- Physics list --------------------------------------------------------------------
  { auto verbosity = 0;     n4::use_our_optical_physics(run_manager.get(), verbosity); }
  // ----- User actions (only generator is mandatory) --------------------------------------
  run_manager -> SetUserInitialization((new n4::actions{generator_messenger.generator()})
    // -> set ((new n4::event_action) -> end(write_vertices))
    -> set ((new n4::event_action) -> begin(show_primary_generator))
    -> set  (new n4::stepping_action{xxxstep})
  );



  // ----- second phase --------------------------------------------------------------------
  ui -> run();

  // user actions, physics_list and detector_description are owned and deleted
  // by the run manager, so they should not be deleted by us.
}

void UI_interactive::spin() {
  if (messenger.spin) {
    // ----- spin the viewport ------------------------------------------------------------
    struct waypoint { float phi; float theta; size_t steps; };
    auto spin_view = [&](auto ui_manager, std::vector<waypoint> data) {
      nain4::silence _{G4cout};
      float phi_start = 0, theta_start = 0;
      for (auto [phi_stop, theta_stop, steps] : data) {
        steps /= messenger.spin_speed;
        auto   phi_d = (  phi_stop -   phi_start) / steps;
        auto theta_d = (theta_stop - theta_start) / steps;
        for (size_t step=0; step<=steps; step++) {
          auto   phi =   phi_start +   phi_d * step;
          auto theta = theta_start + theta_d * step;
          ui_manager->ApplyCommand("/vis/viewer/set/viewpointThetaPhi "
                                   + std::to_string(theta) + ' ' + std::to_string(phi));
        }
        phi_start =   phi_stop;
        theta_start = theta_stop;
      }
    };

    spin_view(ui_manager, {{-400, 205,   1},
                           {-360,  25, 300},
                           {   0,   0, 500}});
  }
}
