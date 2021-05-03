#include "nain4.hh"
#include "g4-mandatory.hh"

#include "geometries/imas.hh"
#include "geometries/nema.hh"
#include "geometries/samples.hh"
#include "geometries/sipm.hh"

#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>

#include <FTFP_BERT.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4OpticalPhysics.hh>

#include <G4OpticalPhoton.hh>
#include <Randomize.hh>

#include <memory>
#include <ostream>
#include <string>


using std::make_unique;
using std::unique_ptr;


template<class M, class K>
bool contains(M const& map, K const& key) { return map.find(key) != end(map); }

// ================================================================================

#include <set>

struct hmm {
  void add(double t) { ts.insert(t); }
  std::multiset<double> ts;

  friend std::ostream& operator<<(std::ostream& out, hmm const& h) {
    using std::setw;
    auto const& m = h.ts;
    auto first =  *cbegin(m) / ps;
    auto last  = *crbegin(m) / ps;
    auto width = last - first;
    out << setw(8) << width << " / " << setw(6) << m.size() << ' '
        << last << " - " << first;
    return out;
  }
};

int main(int argc, char** argv) {

  // Detect interactive mode (if no arguments) and define UI session
  auto ui = argc == 1
    ? make_unique<G4UIExecutive>(argc, argv)
    : unique_ptr <G4UIExecutive>{nullptr};

  // Construct the default run manager
  auto run_manager = unique_ptr<G4RunManager>
    {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial)};

  // For use with phantom_in_cylinder
  auto phantom = build_nema_phantom{}
    .activity(5)
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

  std::map<size_t, hmm> times;

  auto add_to_waveforms = [&times](auto sensor_id, auto time) {
    if (!contains(times, sensor_id)) {
      times.emplace(sensor_id, hmm{});
    }
    auto& [_, map] = *times.find(sensor_id);
    map.add(time);
  };

  // clang-format off
  n4::sensitive_detector::process_hits_fn make_noise = [&add_to_waveforms](G4Step* step) {
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

  n4::sensitive_detector::end_of_event_fn eoe = [&times](auto) {
    using std::setw;
    // TODO persist waveforms
    std::cout << n4::event_number() << std::endl;
    for (auto& [sensor_id, hmm] : times) {
      auto const& ts = hmm.ts;
      if (ts.size() < 10) { continue; }
      auto start = *cbegin(ts);
      std::vector<double> t{};
      // TODO reserve space once good size is known from statistics
      std::copy_if(cbegin(ts), cend(ts), back_inserter(t),
                   [start](auto t) { return t < start + 1000 * ps; });
      size_t a = std::count_if(cbegin(ts), cend(ts), [start](auto t) { return t < start +  100 * ps; });
      size_t b = std::count_if(cbegin(ts), cend(ts), [start](auto t) { return t < start + 1000 * ps; });
      std::cout << sensor_id << " : "
                << setw(3) << a << " / "
                << setw(3) << b << " / "
                << setw(4) << ts.size() << std::endl;

    }
    times.clear();
  };

  // pick one:
  auto sd = new n4::sensitive_detector{"Noisy_detector", make_noise, eoe};
  //auto sd = nullptr; // If you only want to visualize geometry

  // Set mandatory initialization classes

  // run_manager takes ownership of geometry
  run_manager -> SetUserInitialization(new n4::geometry{[&phantom, sd]() -> G4VPhysicalVolume* {
    // Pick one (ensure that generator (below) is compatible) ...
    return cylinder_lined_with_hamamatsus(200*mm, 350*mm, 40*mm, sd);
    return phantom_in_cylinder(phantom, 200*mm, 40*mm, sd);
    return phantom.geometry();
    return imas_demonstrator(nullptr);
    return square_array_of_sipms(sd);
    return nain4::place(sipm_hamamatsu_blue(true, sd)).now();
  }});

  { // Physics list
    auto verbosity = 1;
    auto physics_list = new FTFP_BERT{verbosity};
    physics_list -> ReplacePhysics(new G4EmStandardPhysics_option4());
    physics_list -> RegisterPhysics(new G4OpticalPhysics{});
    run_manager  -> SetUserInitialization(physics_list);
  } // run_manager owns physics_list

  // User action initialization
  run_manager->SetUserInitialization(new n4::actions{
      new n4::generator{[&phantom](G4Event* event) {
        // Pick one that matches geometry
        generate_back_to_back_511_keV_gammas(event, {}, 0);
        //phantom.generate_primaries(event);

        // auto particle = nain4::find_particle("geantino");
        // auto p = G4ThreeVector{0,1,0} * 7 * eV;
        // double time = 0;
        // //auto vertex =      new G4PrimaryVertex({0, 65*mm, 3*mm}, time);
        // auto vertex =      new G4PrimaryVertex({}, time);
        // auto primary = new G4PrimaryParticle(particle,  p.x(),  p.y(),  p.z());
        // primary -> SetPolarization({1,0,0});
        // vertex->SetPrimary(primary);
        // event -> AddPrimaryVertex(vertex);

      }}});

  // Initialize visualization
  auto vis_manager = make_unique<G4VisExecutive>();
  // G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
  // G4VisManager* visManager = new G4VisExecutive{"Quiet"};
  vis_manager -> Initialize();

  // Get the pointer to the User Interface manager
  auto ui_manager = G4UImanager::GetUIpointer();

  // Process macro or start UI session
  if (!ui) {
    // batch mode
    G4String command = "/control/execute ";
    G4String file_name = argv[1];
    ui_manager -> ApplyCommand(command + file_name);
  } else {
    // interactive mode
    {
      ui_manager -> ApplyCommand("/control/execute init_vis.mac");
      ui_manager -> ApplyCommand("/PhysicsList/RegisterPhysics G4EmStandardPhysics_option4");
      ui_manager -> ApplyCommand("/PhysicsList/RegisterPhysics G4OpticalPhysics");
      ui_manager -> ApplyCommand("/vis/scene/endOfEventAction accumulate 100");
      //ui_manager -> ApplyCommand("/run/beamOn 1");
      //nain4::silence _{G4cout};
      int PHI = 150; int THETA = 160; int THETAF = 175;
      auto view = [&ui_manager](auto theta, auto phi) {
        ui_manager->ApplyCommand("/vis/viewer/set/viewpointThetaPhi "
                                 + std::to_string(theta) + ' ' + std::to_string(phi));
      };
      // {
      //   nain4::silence _{G4cout};
      //   for (int phi  =PHI  ; phi  <360+PHI   ; phi  +=4) { view(THETA, phi); }
      //   for (int theta=THETA; theta<360+THETAF; theta+=4) { view(theta, PHI); }
      // }
      ui -> SessionStart();
    }
  }

  // Job termination
  // Free the store: user actions, physics_list and detector_description are
  // owned and deleted by the run manager, so they should not be deleted
  // in the main() program !
}
