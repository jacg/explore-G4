// clang-format off
#include "nain4.hh"
#include "g4-mandatory.hh"
#include "random/random.hh"

#include "geometries/compare_scintillators.hh"
#include "geometries/imas.hh"
#include "geometries/jaszczak.hh"
#include "geometries/nema.hh"
#include "geometries/samples.hh"
#include "geometries/sipm.hh"
#include "messengers/abracadabra.hh"
#include "messengers/density_map.hh"
#include "messengers/generator.hh"
#include "utils/map_set.hh"

#include <G4ClassificationOfNewTrack.hh>
#include <G4RunManager.hh>
#include <G4RunManagerFactory.hh>
#include <G4StackManager.hh>
#include <G4String.hh>
#include <G4SystemOfUnits.hh>
#include <G4Tubs.hh>
#include <G4Types.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UIExecutive.hh>
#include <G4UImanager.hh>
#include <G4VisExecutive.hh>
#include <G4VisManager.hh>

#include <G4OpticalPhoton.hh>
#include <G4Gamma.hh>
#include <Randomize.hh>

#include <cstddef>
#include <functional>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <memory>
#include <ostream>
#include <string>
#include <variant>

using std::make_unique;
using std::unique_ptr;
using std::cout;
using std::endl;
using std::setw;

namespace report_progress {
  G4int n_events_requested = 0;
  std::chrono::steady_clock::time_point events_start;
  std::function<void(int)> report_event_number = [] (int) {
    cout << "No user signal event handler has been set so far" << endl;
  };
  void signal_handler(int signal) { report_event_number(signal); }
  void print_vertex(size_t event_id, G4int id, G4int parent,
                    G4double x, G4double y, G4double z, G4double r,
                    G4double moved, G4double pre_KE, G4double pst_KE, G4double dep_E,
                    G4String const& process_name, G4String const& volume_name,
                    size_t& header_last_printed, bool& track_1_printed_this_event) {
    if (event_id != header_last_printed) {
      cout << "   event  parent  id            x    y    z     r     moved    preKE pstKE  dKE   deposited"
           << endl;
      header_last_printed = event_id;
      track_1_printed_this_event = false;
    }

    if (id == 1 && ! track_1_printed_this_event) {
      track_1_printed_this_event = true;
      cout << endl;
    }

    #define SETW(w,v) setw(w) << v
    #define ROUND(p,v) std::setw(p) << (int) std::round(v)
    cout << std::setprecision(1) << std::fixed;
    cout << SETW(9, event_id)
         << SETW(5, parent) << ' '
         << SETW(5, id)
         << SETW(6, process_name)
         << "  ("
         << ROUND(5,x) << ROUND(5,y) << ROUND(5,z) << " :" << ROUND(4,r)
         << ") "
         << SETW(7, moved) << "   "
         << SETW(6, pre_KE)
         << SETW(6, pst_KE)
         << SETW(6, pre_KE-pst_KE)
         << SETW(6, dep_E)
         << SETW(14, volume_name) << ' '
         << endl;
    #undef ROUND
    #undef SETW
  }

}

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
void stop_if_failed(G4int status) { if (status != 0) { FATAL("A messenger failed"); } }

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
  if (argc  < 2) { FATAL("A model macro file must be provided"); }
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

// Find the inner and outer radii of the LXe layer (will crash for any of the
// detector geometries that do not contain a G4Tubs volumes called "LXe" and
// "Steel_1")
std::tuple<G4double, G4double> find_LXe_inner_and_outer_radii() {
  auto   LXe = n4::find_logical("LXe");
  auto steel = n4::find_logical("Steel_1");
  auto solid_x =   LXe -> GetSolid();
  auto solid_s = steel -> GetSolid();
  auto tubs_x = dynamic_cast<G4Tubs*>(solid_x);
  auto tubs_s = dynamic_cast<G4Tubs*>(solid_s);
  auto LXe_R =  tubs_x -> GetOuterRadius();
  auto LXe_r =  tubs_s -> GetOuterRadius();
  std::cout << "\n\n\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n\n";
  std::cout << "LXe radii: " << LXe_r << ' ' << LXe_R;
  std::cout << "\n\n\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n\n";
  return {LXe_r, LXe_R};
};

// ============================== MAIN =======================================================
int main(int argc, char** argv) {

  abracadabra_messenger messenger;
  auto current_event = [&]() { return n4::event_number() + messenger.offset; };


  report_progress::report_event_number = [&](int) {
    auto n = n4::event_number();
    auto N = report_progress::n_events_requested;
    auto fraction = static_cast<float>(n) / N;
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = now - report_progress::events_start;
    auto seconds = elapsed_seconds.count();
    auto eta = static_cast<unsigned>(seconds / fraction - seconds);
    auto eta_hours   =  eta / 3600;
    auto eta_minutes = (eta % 3600) / 60;
    auto eta_seconds =  eta %   60;

    cout << "Processing event number " << n << " / " << N
         << std::setprecision(1) << std::fixed << " ("  << 100 * fraction << " %)"
         << " after " << seconds << " s. ETA: "
         << eta_hours << "h " << eta_minutes << "m " << eta_seconds << "s\n";
  };
  std::signal(SIGUSR1, report_progress::signal_handler);


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
  auto open_writer = [&writer, &messenger]() { writer.reset(new hdf5_io{messenger.outfile});};

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
        if (t - trigger_time > acquisition_widow) { break; }
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

  // ----- Some phantoms' generators require access to run manager, which won't be created until later
  unique_ptr<G4RunManager> run_manager{};
  // ----- Available phantoms -----------------------------------------------------------
  auto sanity   = [            ] { return sanity_check_phantom(); };

  auto jaszczak = [&run_manager, &messenger] {
    return build_jaszczak_phantom(run_manager, messenger.vacuum_phantom)
      .sphere_activity(messenger.jaszczak_activity_sphere)
      .  body_activity(messenger.jaszczak_activity_body)
      .   rod_activity(messenger.jaszczak_activity_rod)
      .build();
  };

  auto nema_3 = [&messenger]() {
    auto fov_length = messenger.cylinder_length * mm;
    return nema_3_phantom{fov_length};
  };

  auto nema_4 = [](auto z_offset)                 { return nema_4_phantom(           z_offset); };
  auto nema_5 = [](auto n_sleeves, auto y_offset) { return nema_5_phantom(n_sleeves, y_offset); };

  auto nema_7 = [&messenger]() {
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
      .vacuum_body(messenger.vacuum_phantom)
      .build();
  };

  // ----- Choice of phantom -------------------------------------------------------------
  // Can choose phantom in macros with `/abracadabra/phantom <choice>`
  // The nema_3 phantom's length is determined by `/abracadabra/cylinder_length` in mm

  using polymorphic_phantom = std::variant<nema_3_phantom, nema_4_phantom,
                                           nema_5_phantom, nema_7_phantom,
                                           sanity_check_phantom,
                                           jaszczak_phantom>;

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
    p == "nema_3"   ? phantom = nema_3()                                            :
    p == "nema_4"   ? phantom = nema_4(                         messenger.z_offset) :
    p == "nema_5"   ? phantom = nema_5(messenger.nema5_sleeves, messenger.y_offset) :
    p == "nema_7"   ? phantom = nema_7()                                            :
    p == "sanity"   ? phantom = sanity()                                            :
    p == "jaszczak" ? phantom = jaszczak()                                          :
    (throw (FATAL(("Unrecoginzed phantom: " + p).c_str()), "see note 1 in nain4.hh"));
  };

  // ----- Available detector geometries -------------------------------------------------
  // Can choose detector in macros with `/abracadabra/detector <choice>`
  auto detector = [&, &d = messenger.detector]() -> G4VPhysicalVolume* {
    auto dr_sci = messenger.scintillator_thickness  * mm;
    auto dr_Qtz = messenger.quartz_thickness        * mm;
    auto length = messenger.cylinder_length         * mm;
    auto radius = messenger.cylinder_radius         * mm;
    auto clear  = messenger.steel_is_vacuum;
    auto magic  = (d == "scintillator") ? 1 : messenger.magic_level;
    auto scint  = messenger.scintillator;
    return
      d == "scintillator" ? compare_scintillators(scint  , length, radius, dr_sci)     :
      d == "cylinder"     ? cylinder_lined_with_hamamatsus(length, radius, dr_sci, sd) :
      d == "imas"         ? imas_demonstrator(sd, length, dr_Qtz, dr_sci, clear)       :
      magic >= 3          ? magic_detector()                                           :
      d == "square"       ? square_array_of_sipms(sd)                                  :
      d == "hamamatsu"    ? nain4::place(sipm_hamamatsu_blue(true, sd)).now()          :
      (FATAL(("Unrecoginzed detector: " + d).c_str()), nullptr);
  };

  // ----- Should the geometry contain phantom only / detector only / both
  // Can choose geometry in macros with `/abracadabra/geometry <choice>`
  auto geometry = [&, &g = messenger.geometry]() -> G4VPhysicalVolume* {
    return
      g == "detector" ? detector()         :
      g == "phantom"  ? phantom_geometry() :
      g == "both"     ? n4::combine_geometries(phantom_geometry(), detector()) :
      (FATAL(("Unrecoginzed geometry: " + g).c_str()), nullptr);
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

  // If messenger.E_cut is set, save time by not simulating secondaries for
  // events in which a gamma's energy falls below the cut, before entering LXe.
  // Such events will be rejected later on, on the grounds of not registering
  // enough energy, so there is no point in wasting time on the very expensive
  // simulation of secondaries.
  G4double lowest_pre_LXe_gamma_energy_in_event;

  // If either gamma fails to interact in LXe, the event is also boring, and we
  // can skip the secondaries.
  bool detected_gamma_1, detected_gamma_2;

  // Inner and outer radii of the LXe layer
  G4double LXe_r, LXe_R; // Initialized in start_run

  n4::stepping_action::action_t stepping_action = [&](auto step) {
    static size_t header_last_printed = 666;
    static bool track_1_printed_this_event = false;

    auto pst_pt = step -> GetPostStepPoint();
    auto pre_pt = step -> GetPreStepPoint();

    auto track = step -> GetTrack();
    auto process_name    = transp(pst_pt -> GetProcessDefinedStep() -> GetProcessName());
    G4String volume_name;

    const auto GAMMA = G4Gamma::Definition();

    if (messenger.magic_level < 2) {
      // ----- Real detector ------------------------------------------------------------------------
      // Only record vertices (not transport) of gammas
      auto particle = track -> GetParticleDefinition();
      if (particle != GAMMA || process_name == "---->") return;
      volume_name = pst_pt -> GetPhysicalVolume() -> GetName();
    } else {
      // ----- Magic LXe detector --------------------------------------------------------------------
      // 1. Immediately stop any particle that reaches LXe.
      // 2. Record only (a) gammas (b) which have reached LXe
      auto pst_physvol = pst_pt -> GetPhysicalVolume();
      volume_name = pst_physvol ? pst_physvol -> GetName() : "None";
      // Stop as soon as LXe reached
      if (volume_name == "LXe") { track -> SetTrackStatus(G4TrackStatus::fStopAndKill); }
      // Write only gammas entering LXe (not expecting anything other than gamma, before LXe)
      if (volume_name != "LXe" || process_name != "---->" ) return;
    }

    // Event and particle identities
    auto event_id = current_event();
    auto id     = track -> GetTrackID();
    auto parent = track -> GetParentID();

    // Position, motion and timing
    auto pos = pst_pt -> GetPosition();
    auto x = pos.x(); auto y = pos.y(); auto z = pos.z(); auto r = sqrt(x*x + y*y);
    auto t = pst_pt -> GetGlobalTime();
    auto moved = step -> GetDeltaPosition().mag();

    // Energy
    auto pre_KE = pre_pt -> GetKineticEnergy()      / keV;
    auto pst_KE = pst_pt -> GetKineticEnergy()      / keV;
    auto dep_E  = step   -> GetTotalEnergyDeposit() / keV;

    // Bookkeeping for gamma energies that might fall below messenger.E_cut
    if (r < LXe_r) {
      lowest_pre_LXe_gamma_energy_in_event = std::min(lowest_pre_LXe_gamma_energy_in_event, pst_KE);
      if (messenger.verbosity > 3) {
        std::cout << " gamma low: " << lowest_pre_LXe_gamma_energy_in_event << std::endl;
      }
    } else if (volume_name == "LXe") {
      if (id == 1) { detected_gamma_1 = true; }
      if (id == 2) { detected_gamma_2 = true; }
    }

    // Process and volume ids
    auto  volume_id =  volume_names.id( volume_name);
    auto process_id = process_names.id(process_name);

    // Write vertex to output file
    writer -> write_vertex(       event_id, id, parent, x,y,z,t, moved, pre_KE, pst_KE, dep_E,
                                  process_id,   volume_id);

    // Live progress report on stdout
    if (messenger.verbosity < 2) return;
    report_progress::print_vertex(event_id, id, parent, x,y,z,r, moved, pre_KE, pst_KE, dep_E,
                                  process_name, volume_name,
                                  header_last_printed, track_1_printed_this_event);
  };

  // BeginOfEvent action:
  // 1. Resets lowest gamma energy bookkeeping
  // 2. Writes the primary vertex of the event to HDF5
  n4::event_action::action_t begin_event = [&](auto event) {
    // Reset event bookkeeping
    lowest_pre_LXe_gamma_energy_in_event = 511.0;
    trigger_time                         = std::numeric_limits<G4double>::infinity();
    detected_gamma_1 = false;
    detected_gamma_2 = false;
    // Write primary vertex
    using std::setw;
    auto event_id = current_event();
    auto vertex = event -> GetPrimaryVertex();
    auto pos = vertex -> GetPosition();
    auto mom = vertex -> GetPrimary() -> GetMomentum();
    auto [ x, y, z] = std::make_tuple(pos.x(), pos.y(), pos.z());
    auto [px,py,pz] = std::make_tuple(mom.x(), mom.y(), mom.z());
    writer -> write_primary(event_id, x,y,z, px,py,pz);
    if (messenger.verbosity < 1) { return; }
    if (messenger.verbosity < 2) {
      cout << std::setprecision(1) << std::fixed;
      cout << setw(9) << event_id << endl;
      return;
    }
    cout << endl << setw(9) << event_id << " -------  "
         << setw(7) <<  x << setw(7) <<  y << setw(7) <<  z << "     "
         << setw(7) << px << setw(7) << py << setw(7) << pz
         << "  --------------------------------" << endl;
  };

  size_t secondaries_no = 0, secondaries_yes = 0;
  n4::run_action::action_t   end_run = [&](auto) {
    writer -> write_strings("process_names", process_names.items_ordered_by_id());
    writer -> write_strings( "volume_names",  volume_names.items_ordered_by_id());
    std::cout << "Scondaries simulated " << secondaries_yes
              << " times, ignored " << secondaries_no << " times (" << std::setprecision(0)
              << 100.0 * secondaries_yes / (secondaries_yes + secondaries_no)<< " %)\n";
  };

  n4::run_action::action_t start_run = [&](auto run) {
    report_progress::events_start = std::chrono::steady_clock::now();
    report_progress::n_events_requested = run -> GetNumberOfEventToBeProcessed();
    std::tie(LXe_r, LXe_R) = find_LXe_inner_and_outer_radii();
  };

  // ----- Stacking: Process gammas before secondaries (secondaries only if needed) -------
  unsigned stage; // 1: gammas; 2: secondaries

  n4::stacking_action::classify_t kill_or_wait_secondaries = [&stage, &messenger](auto track) {
    const auto NOW  = G4ClassificationOfNewTrack::fUrgent;
    const auto KILL = G4ClassificationOfNewTrack::fKill;
    const auto WAIT = G4ClassificationOfNewTrack::fWaiting;

    const bool verbose = messenger.verbosity > 4;
    auto  vNOW = [=] { if (verbose) {std::cout <<  "NOW\n";} return  NOW; };
    auto vKILL = [=] { if (verbose) {std::cout << "KILL\n";} return KILL; };
    auto vWAIT = [=] { if (verbose) {std::cout << "WAIT\n";} return WAIT; };

    if (stage == 1) { // primary gammas only, delay secondaries
      if (verbose) {
        std::cout << track -> GetParentID() << " -> " << track -> GetTrackID() << ' '
          << track -> GetDefinition() -> GetParticleName() << "     ";
      }
      bool is_primary         = track -> GetParentID() == 0;
      bool ignore_secondaries = messenger.magic_level > 0;
      if (is_primary)         { return vNOW();  }
      if (ignore_secondaries) { return vKILL(); } else { return vWAIT(); }

    } else if (stage == 2) { // secondaries
      return                            NOW;

    } else {
      FATAL(("FUNNY STAGE: " + std::to_string(stage)).c_str());
    }
  };

  n4::stacking_action::voidvoid_t reset_stage_no = [&stage] { stage = 1; /*std::cout << "RESET TO STAGE 1\n";*/ };

  n4::stacking_action::stage_t forget_or_track_secondaries = [&] (G4StackManager * const stack_manager) {
    stage++;
    if (stage == 2) {
      bool ignore_secondaries = messenger.magic_level                > 0               ||
                                lowest_pre_LXe_gamma_energy_in_event < messenger.E_cut ||
                                ! detected_gamma_1 || ! detected_gamma_2;
      if (messenger.verbosity > 2) {
        std::cout << "\nignore secondaries: " << (ignore_secondaries ? "YES" : "NO ") << "   "
                  << lowest_pre_LXe_gamma_energy_in_event << " <? " << messenger.E_cut
                  << "   gammas detected: " << std::boolalpha << detected_gamma_1 << ' ' <<  detected_gamma_2
                  << "\n\n";}
      if (ignore_secondaries) { stack_manager -> clear();                                   secondaries_no  += 1; }
      else { /* do nothing, and everything from waiting is automatically moved to urgent */ secondaries_yes += 1; }
    }
  };

  // ===== Mandatory G4 initializations ===================================================

  // Construct the default run manager
  run_manager = unique_ptr<G4RunManager> {G4RunManagerFactory::CreateRunManager(G4RunManagerType::Serial)};

  // ----- Geometry (run_manager takes ownership) -----------------------------------------
  run_manager -> SetUserInitialization(new n4::geometry{[&write_sensor_database, geometry]() -> G4VPhysicalVolume* {
    return write_sensor_database(geometry());
  }});
  // ----- Physics list --------------------------------------------------------------------
  { auto verbosity = 0;     n4::use_our_optical_physics(run_manager.get(), verbosity); }
  // ----- User actions (only generator is mandatory) --------------------------------------
  auto actions = (new n4::actions{generator_messenger.generator()})
    -> set ((new n4::run_action)      -> begin(start_run)
                                      -> end  (  end_run))
    -> set ((new n4::event_action)    -> begin(begin_event))
    -> set ((new n4::stacking_action) -> classify  (   kill_or_wait_secondaries)
                                      -> next_stage(forget_or_track_secondaries)
                                      -> next_event(reset_stage_no))
    -> set  (new n4::stepping_action{stepping_action});

  run_manager -> SetUserInitialization(actions);
  // ----- Construct density map if requested ------------------------------------------
  density_map_messenger density_map_messenger{run_manager.get()};

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
