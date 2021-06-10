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
#include <Randomize.hh>

#include <memory>
#include <ostream>
#include <string>

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
    messenger -> DeclareProperty("spin_view" , spin      , "Spin geometry view");
    messenger -> DeclareProperty("spin_speed", spin_speed, "Spin geometry speed");
    messenger -> DeclareProperty("xenon_thickness", xenon_thickness, "Thickness of LXe layer");
    messenger -> DeclareProperty("cylinder_length", cylinder_length, "Length of cylinder");
    messenger -> DeclareProperty("cylinder_radius", cylinder_radius, "Radius of cylinder");
  }
  size_t offset = 0;
  G4String outfile    = "default_out.h5";
  G4String geometry   = "phantom";
  bool     spin       = true;
  G4int    spin_speed = 10;
  G4double xenon_thickness =  40 * mm;
  G4double cylinder_length = 600 * mm;
  G4double cylinder_radius = 200 * mm;
private:
  unique_ptr<G4GenericMessenger> messenger;
};

struct generator_messenger : G4UImessenger {
  generator_messenger(n4::generator::function& generate, std::map<G4String, n4::generator::function>& choices)
    : dir{new G4UIdirectory     ("/generator/")}
    , cmd{new G4UIcmdWithAString("/generator/choose", this)}
    , generate{generate}
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
private:
  unique_ptr<G4UIdirectory> dir;
  unique_ptr<G4UIcmdWithAString> cmd;
  n4::generator::function& generate;
  std::map<G4String, n4::generator::function>& choices;
};

// ----- UI: Abstract class with two concrete implementations: interactive and batch -----------
struct UI {
  static UI* make(int argc, char** argv, abracadabra_messenger&); // Polymorphic constructor
  UI(char** argv, abracadabra_messenger& messenger) : messenger{messenger} {
      G4String model_filename = argv[1];
      ui_manager -> ApplyCommand("/control/execute " + model_filename);
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
    ui_manager -> ApplyCommand("/control/execute init_vis.mac");
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
  void run() { ui_manager -> ApplyCommand("/control/execute " + run_macro_filename); }
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

// ============================== MAIN =======================================================
int main(int argc, char** argv) {

  abracadabra_messenger messenger;

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

  n4::sensitive_detector::end_of_event_fn write_hits = [&times, &writer, &messenger](auto) {
    size_t event_id = n4::event_number() + messenger.offset;
    std::cout << event_id << std::endl;
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

  // ----- A variety of geometries to choose from, for experimentation -------------------

  // some of the available geometries use this phantom
  auto phantom = build_nema_phantom{}
    .activity(0)
    .length(140*mm)
    .inner_radius(114.4*mm)
    .outer_radius(152.0*mm)
    .sphere(10*mm / 2, 20)
    .sphere(13*mm / 2, 20)
    .sphere(17*mm / 2, 20)
    .sphere(22*mm / 2, 20)
    .sphere(28*mm / 2, 0)
    .sphere(37*mm / 2, 0)
    .build();

  // Can choose geometry in macros with `/abracadabra/geometry <choice>`
  auto geometry = [&, &g = messenger.geometry]() -> G4VPhysicalVolume* {
    auto dr_LXe = messenger.xenon_thickness * mm;
    auto length = messenger.cylinder_length * mm;
    auto radius = messenger.cylinder_radius * mm;
    return
      g == "phantom"             ? phantom.geometry() :
      g == "cylinder"            ? cylinder_lined_with_hamamatsus(length, radius, dr_LXe, sd) :
      g == "phantom_in_cylinder" ? phantom_in_cylinder(phantom,   length, radius, dr_LXe, sd) :
      g == "imas"                ? imas_demonstrator(sd) :
      g == "square"              ? square_array_of_sipms(sd) :
      g == "hamamatsu"           ? nain4::place(sipm_hamamatsu_blue(true, sd)).now() :
      throw "Unrecoginzed geometry " + g;
  };

  // ----- A choice of generators ---------------------------------------------------------
  // Can choose generator in macros with `/abracadabra/generator <choice>`
  std::map<G4String, n4::generator::function> generators = {
    {"origin"      , [ ](auto event) { generate_back_to_back_511_keV_gammas(event, {}, 0); }},
    {"phantom"     , [&](auto event) { phantom.generate_primaries(event); }},
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

  // Generator is passed to run manager before the specific choice of generator
  // is read from the configuration file, so we add an indirection which allows
  // us to use the actual generator which is chosen later.
  // TODO can we avoid these contortions by initialising run manager after
  // reading the UI macros?
  n4::generator::function chosen_generator = [](auto) { throw "No generator has been chosen"; };
  n4::generator::function generator = [&chosen_generator](auto event) { chosen_generator(event); };
  generator_messenger generator_messenger{chosen_generator, generators};

  UI* ui = UI::make(argc, argv, messenger);

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
  run_manager -> SetUserInitialization(new n4::actions{generator});


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
