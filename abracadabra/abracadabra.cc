// clang-format off
#include "nain4.hh"
#include "g4-mandatory.hh"
#include "random/random.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/samples.hh"
#include "geometries/sipm.hh"
#include "messengers/abracadabra.hh"
#include "messengers/attenuation_map.hh"
#include "messengers/generator.hh"
#include "utils/map_set.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>

#include <G4OpticalPhoton.hh>
#include <G4Gamma.hh>
#include <Randomize.hh>

#include <memory>
#include <ostream>
#include <string>
#include <variant>

using std::make_unique;
using std::unique_ptr;
using std::cout;
using std::endl;
using std::setw;

#include <set>
using times_set = std::multiset<double>;

// NB: I can't help feeling that we're reinventing a wheel that HDF5 should
// already be implementing for us, but I haven't managed to find it in the
// documentation:

// Some of our HDF5 tables need to store strings whose values repeat many times.
// For example, we expect only 3 different processes at our LXe vertices (Rayl,
// compt, phot), but they will be stored *many* times. Similarly, the name of
// the volume in which a step terminates, can take on a limited number of
// values, but will be recorded in each vertex. Rather storing a copy each time,
// it's much more memory-efficient to store each name once, along with a unique
// identifier int, and store that int instead of the string itself. For this we
// need a utility which can tell us the id of any such string, creating new ids
// on the fly, for values we haven't seen before, and retrieving the already
// assigned ids of values that we have seen before.
template<class T>
class id_store {
public:
  id_store(const std::initializer_list<T>& data) { for (auto& item : data) { id(item); } }
  size_t id(const T& item) {
    auto found = ids.find(item);
    if (found != ids.end()) { // Known item: return its id
      return found->second;
    } else { // New item: insert into store and return new id
      auto this_id = items.size();
      ids.insert({item, this_id});
      items.push_back(item);
      return this_id;
    }
  }
  const std::vector<T>& items_ordered_by_id() const { return items; }
private:
  std::map<T, size_t> ids;
  std::vector<T> items;
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
  auto trigger_time = std::numeric_limits<G4double>::infinity();
  n4::sensitive_detector::process_hits_fn store_hits = [&add_to_waveforms, &trigger_time](G4Step* step) {
    static auto optical_photon = G4OpticalPhoton::Definition();

    auto track    = step -> GetTrack();
    auto particle = track -> GetParticleDefinition();
    auto name     = particle -> GetParticleName();

    if (particle == optical_photon) {
      auto time = track -> GetGlobalTime();
      auto sensor_id = step -> GetPreStepPoint() -> GetTouchable() -> GetCopyNumber(1);
      add_to_waveforms(sensor_id, time);
      trigger_time = std::min(trigger_time, time);
      return true;
    }

    return false; // Still not *entirely* sure about the return value meaning
  };

  n4::sensitive_detector::end_of_event_fn write_hits = [&](auto) {
    const auto acquisition_widow = 500 * ns;
    size_t event_id = current_event();
    for (auto& [sensor_id, ts] : times) {
      std::vector<float> tvec;
      for (auto t : ts) {
        t -= trigger_time;
        if (t > acquisition_widow) { break; }
        tvec.push_back(t);
      }
      if ( ! tvec.empty()) {
        writer -> write_waveform    (event_id, sensor_id, tvec);
        writer -> write_total_charge(event_id, sensor_id, tvec.size());
      }
    }
    times.clear();
  };

  // pick one:
  auto sd = new n4::sensitive_detector{"Writing_detector", store_hits, write_hits};
  //auto sd = nullptr; // If you only want to visualize geometry

  // ----- Available phantoms -----------------------------------------------------------

  auto nema_3 = [&messenger]() {
    auto fov_length = messenger.cylinder_length * mm;
    return nema_3_phantom{fov_length};
  };

  auto nema_4 = [](auto z_offset)                 { return nema_4_phantom(           z_offset); };
  auto nema_5 = [](auto n_sleeves, auto y_offset) { return nema_5_phantom(n_sleeves, y_offset); };

  auto nema_7 = []() {
    return build_nema_7_phantom{}
      .activity(1)
      .length(180*mm)
      .inner_diameter(114.4*mm)
      .top_radius    (147.0*mm)
      .corner_radius ( 77.0*mm)
      .lungD(50*mm)
      .sphereD(10*mm, 4)
      .sphereD(13*mm, 4)
      .sphereD(17*mm, 4)
      .sphereD(22*mm, 4)
      .sphereD(28*mm, 4)
      .sphereD(37*mm, 4)
      .build();
  };

  // ----- Choice of phantom -------------------------------------------------------------
  // Can choose phantom in macros with `/abracadabra/phantom <choice>`
  // The nema_3 phantom's length is determined by `/abracadabra/cylinder_length` in mm

  using polymorphic_phantom = std::variant<nema_3_phantom, nema_4_phantom,
                                           nema_5_phantom, nema_7_phantom>;

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
    p == "nema_3" ? phantom = nema_3()                                            :
    p == "nema_4" ? phantom = nema_4(                         messenger.z_offset) :
    p == "nema_5" ? phantom = nema_5(messenger.nema5_sleeves, messenger.y_offset) :
    p == "nema_7" ? phantom = nema_7()                                            :
    throw "Unrecoginzed phantom " + p;
  };

  // ----- Available detector geometries -------------------------------------------------
  // Can choose detector in macros with `/abracadabra/detector <choice>`
  auto detector = [&, &d = messenger.detector]() -> G4VPhysicalVolume* {
    auto dr_LXe = messenger.xenon_thickness  * mm;
    auto dr_Qtz = messenger.quartz_thickness * mm;
    auto length = messenger.cylinder_length  * mm;
    auto radius = messenger.cylinder_radius  * mm;
    auto clear  = messenger.vac_pre_lxe;
    return
      d == "cylinder"  ? cylinder_lined_with_hamamatsus(length, radius, dr_LXe, sd) :
      d == "imas"      ? imas_demonstrator(sd, length, dr_Qtz, dr_LXe, clear)       :
      d == "square"    ? square_array_of_sipms(sd)                                  :
      d == "hamamatsu" ? nain4::place(sipm_hamamatsu_blue(true, sd)).now()          :
      throw "Unrecoginzed detector " + d;
  };
  // ----- Should the geometry contain phantom only / detector only / both
  // Can choose geometry in macros with `/abracadabra/geometry <choice>`
  auto geometry = [&, &g = messenger.geometry]() -> G4VPhysicalVolume* {
    return
      g == "detector" ? detector()         :
      g == "phantom"  ? phantom_geometry() :
      g == "both"     ? n4::combine_geometries(phantom_geometry(), detector()) :
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

  id_store<std::string> process_names{{"compt", "phot", "Rayl"}};
  id_store<std::string>  volume_names{
    {"LXe", // Ensure that LXe has id 0: the rest in inside-out order
     "Cavity", "Steel_0", "Inner_vacuum", "Steel_1", "Quartz","Outer_vacuum", "Steel_2",
     // NEMA7 phantom parts (Source_N also used by NEMA3)
     "Body", "Lung", "Source_0", "Source_1", "Source_2", "Source_3", "Source_4", "Source_5",
     // NEMA4
     "Cylinder", "Line_source",
     // NEMA5
     "Source", "Sleeves"}};

  n4::stepping_action::action_t write_vertex = [&](auto step) {
    static size_t previous_event = 666;
    static size_t header_last_printed = 666;
    static bool track_1_printed_this_event = false;
    auto pst_pt = step -> GetPostStepPoint();
    auto track = step -> GetTrack();
    auto particle = track -> GetParticleDefinition();
    if (particle == G4Gamma::Definition()) {
      auto id = track -> GetTrackID();
      auto pre_pt = step -> GetPreStepPoint();
      auto event_id = current_event();
      previous_event = event_id;
      auto parent = track -> GetParentID();
      auto pos = pst_pt -> GetPosition();
      auto x = pos.x(); auto y = pos.y(); auto z = pos.z(); auto r = sqrt(x*x + y*y);
      auto moved = step -> GetDeltaPosition().mag();
      auto dep_E = step -> GetTotalEnergyDeposit() / keV;
      auto volume_name = pre_pt -> GetPhysicalVolume() -> GetName();
      auto pre_KE = pre_pt -> GetKineticEnergy() / keV;
      auto pst_KE = pst_pt -> GetKineticEnergy() / keV;
      auto process_name = transp(pst_pt -> GetProcessDefinedStep() -> GetProcessName());

      if (process_name == "---->") return;
      auto t = pst_pt -> GetGlobalTime();
      auto process_id = process_names.id(process_name);
      auto  volume_id =  volume_names.id( volume_name);
      writer -> write_vertex(event_id, id, parent, x, y, z, t, moved, pre_KE, pst_KE, dep_E, process_id, volume_id);

      if (!messenger.print) return;

      if (event_id != header_last_printed) {
        cout << "   event  parent  id            x    y    z     r     moved    preKE pstKE   deposited" << endl;
        cout << endl;
        header_last_printed = event_id;
        track_1_printed_this_event = false;
      }

      if (id == 1 && ! track_1_printed_this_event) {
        track_1_printed_this_event = true;
        cout << endl;
      }

      cout << std::setprecision(1) << std::fixed;
      cout << setw(9) << event_id
           << setw(5) << parent << ' '
           << setw(5) << id
           << setw(6) << process_name
           << "  (" << std::setw(5) << (int)x << std::setw(5) << (int)y << std::setw(5) << (int)z << " :" << std::setw(4) << (int)r << ") "
           << setw(7) << moved << "   "
           << setw(6) << pre_KE
           << setw(6) << pst_KE
           << setw(6) << dep_E
           << setw(20) << volume_name << ' '
           << endl;
    }
  };

  n4::event_action::action_t write_primary_generator = [&](auto event) {
    using std::setw;
    auto event_id = current_event();
    auto vertex = event -> GetPrimaryVertex();
    auto pos = vertex -> GetPosition();
    auto mom = vertex -> GetPrimary() -> GetMomentum();
    auto [ x, y, z] = std::make_tuple(pos.x(), pos.y(), pos.z());
    auto [px,py,pz] = std::make_tuple(mom.x(), mom.y(), mom.z());
    writer -> write_primary(event_id, x,y,z, px,py,pz);
    cout << std::setprecision(1) << std::fixed;
    cout << setw(9) << event_id;
    if (!messenger.print) { cout << endl; return; }
    cout << " -----------------  "
         << setw(7) <<  x << setw(7) <<  y << setw(7) <<  z << "     "
         << setw(7) << px << setw(7) << py << setw(7) << pz
         << "  ----------------------" << std::endl;
  };

  n4::run_action::action_t write_string_tables = [&](auto) {
    writer -> write_strings("process_names", process_names.items_ordered_by_id());
    writer -> write_strings( "volume_names",  volume_names.items_ordered_by_id());
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
    -> set ((new n4::event_action) -> begin(write_primary_generator))
    -> set  (new n4::stepping_action{write_vertex})
    -> set ((new n4::run_action) -> end(write_string_tables))
  );
  // ----- Construct attenuation map if requested ------------------------------------------
  attenuation_map_messenger attenuation_map_messenger{run_manager.get()};
  // ----- second phase --------------------------------------------------------------------
  ui -> run();

  // user actions, physics_list and detector_description are owned and deleted
  // by the run manager, so they should not be deleted by us.
}

// ================ Utility for spinning the visualized geometry ==========================
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
