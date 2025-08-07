#include <bits/stdc++.h>
using namespace std;

const double INF_COST = numeric_limits<double>::max();
const int HOURS_IN_FORECAST = 24;
const int BATTERY_CHARGE_LEVELS = 101;

struct Generator {
    string name;
    double max_output_MW;
    double cost_per_MWh;
    int uptime_hours_remaining;
    bool in_maintenance;
};

struct Battery {
    double capacity_MWh;
    double charge_efficiency;
    double discharge_efficiency;
    double current_charge_MWh;
};

struct SmartGridManager {
    vector<Generator> generators;
    Battery battery;
    bool battery_initialized = false;

    vector<double> forecastDemand(const vector<double>& historical_data) {
        if (historical_data.empty()) {
            cout << "No historical data to forecast.\n";
            return {};
        }

        function<double(const vector<double>&, int, int)> recursive_average =
            [&](const vector<double>& data, int start_idx, int end_idx) -> double {
            if (start_idx == end_idx) {
                return data[start_idx];
            }
            if (start_idx > end_idx) {
                return 0.0;
            }
            int mid = start_idx + (end_idx - start_idx) / 2;
            double left_avg = recursive_average(data, start_idx, mid);
            double right_avg = recursive_average(data, mid + 1, end_idx);
            double total_sum = left_avg * (mid - start_idx + 1) + right_avg * (end_idx - (mid + 1) + 1);
            double total_count = (end_idx - start_idx + 1);
            return total_sum / total_count;
        };

        double overall_avg = recursive_average(historical_data, 0, historical_data.size() - 1);

        vector<double> forecast(HOURS_IN_FORECAST);
        for (int i = 0; i < HOURS_IN_FORECAST; ++i) {
            double daily_pattern_factor = 1.0;
            if (i >= 7 && i <= 10) daily_pattern_factor = 1.2;
            if (i >= 18 && i <= 21) daily_pattern_factor = 1.5;
            forecast[i] = overall_avg * daily_pattern_factor;
        }

        cout << "Demand Forecast generated using D&C smoothing.\n";
        return forecast;
    }

    pair<double, vector<map<string, double>>> optimizeEnergyDispatch(const vector<double>& demand_forecast) {
        if (!battery_initialized || generators.empty() || demand_forecast.empty()) {
            cout << "Error: Grid components not set up or no forecast.\n";
            return {INF_COST, {}};
        }

        vector<vector<double>> dp(HOURS_IN_FORECAST + 1, vector<double>(BATTERY_CHARGE_LEVELS, INF_COST));
        vector<vector<int>> parent_charge_idx(HOURS_IN_FORECAST + 1, vector<int>(BATTERY_CHARGE_LEVELS, -1));
        vector<vector<map<string, double>>> parent_actions(HOURS_IN_FORECAST + 1, vector<map<string, double>>(BATTERY_CHARGE_LEVELS));

        int initial_charge_idx = static_cast<int>(round((battery.current_charge_MWh / battery.capacity_MWh) * (BATTERY_CHARGE_LEVELS - 1)));
        dp[0][initial_charge_idx] = 0.0;

        for (int h = 0; h < HOURS_IN_FORECAST; ++h) {
            double current_demand = demand_forecast[h];

            for (int prev_charge_idx = 0; prev_charge_idx < BATTERY_CHARGE_LEVELS; ++prev_charge_idx) {
                if (dp[h][prev_charge_idx] == INF_COST) continue;

                double prev_charge_MWh = (static_cast<double>(prev_charge_idx) / (BATTERY_CHARGE_LEVELS - 1)) * battery.capacity_MWh;

                for (double battery_action_MWh_raw = -battery.capacity_MWh; battery_action_MWh_raw <= battery.capacity_MWh; battery_action_MWh_raw += 1.0) {
                    battery_action_MWh_raw = max(-battery.capacity_MWh, min(battery.capacity_MWh, battery_action_MWh_raw));

                    double next_charge_MWh = prev_charge_MWh;
                    double battery_contribution_MWh = 0;

                    if (battery_action_MWh_raw > 0) {
                        double actual_charge = battery_action_MWh_raw * battery.charge_efficiency;
                        if (prev_charge_MWh + actual_charge > battery.capacity_MWh + 1e-6) continue;
                        next_charge_MWh = prev_charge_MWh + actual_charge;
                        battery_contribution_MWh = -battery_action_MWh_raw;
                    } else if (battery_action_MWh_raw < 0) {
                        double actual_discharge_needed = -battery_action_MWh_raw;
                        double actual_discharge_provided = actual_discharge_needed * battery.discharge_efficiency;
                        if (prev_charge_MWh - actual_discharge_needed < -1e-6) continue;
                        next_charge_MWh = prev_charge_MWh - actual_discharge_needed;
                        battery_contribution_MWh = actual_discharge_provided;
                    }

                    next_charge_MWh = max(0.0, min(battery.capacity_MWh, next_charge_MWh));
                    int next_charge_idx = static_cast<int>(round((next_charge_MWh / battery.capacity_MWh) * (BATTERY_CHARGE_LEVELS - 1)));
                    next_charge_idx = max(0, min(BATTERY_CHARGE_LEVELS - 1, next_charge_idx));

                    double remaining_demand_for_gens = current_demand - battery_contribution_MWh;

                    double generator_cost_this_hour = 0;
                    double current_generation_MW_sum = 0;
                    map<string, double> dispatch_detail_for_hour;

                    vector<Generator*> sorted_gens;
                    for(auto& gen : generators) {
                        if (!gen.in_maintenance) {
                            sorted_gens.push_back(&gen);
                        }
                    }
                    sort(sorted_gens.begin(), sorted_gens.end(), [](Generator* a, Generator* b) {
                        return a->cost_per_MWh < b->cost_per_MWh;
                    });

                    double demand_to_meet_by_gens = remaining_demand_for_gens;

                    for (const auto& gen_ptr : sorted_gens) {
                        if (demand_to_meet_by_gens <= 0) break;

                        double gen_output = min(gen_ptr->max_output_MW, demand_to_meet_by_gens);
                        generator_cost_this_hour += gen_output * gen_ptr->cost_per_MWh;
                        current_generation_MW_sum += gen_output;
                        dispatch_detail_for_hour[gen_ptr->name] = gen_output;
                        demand_to_meet_by_gens -= gen_output;
                    }

                    if (demand_to_meet_by_gens > 1e-6) {
                        continue;
                    }

                    double total_cost_this_step = dp[h][prev_charge_idx] + generator_cost_this_hour;

                    if (total_cost_this_step < dp[h + 1][next_charge_idx]) {
                        dp[h + 1][next_charge_idx] = total_cost_this_step;
                        parent_charge_idx[h + 1][next_charge_idx] = prev_charge_idx;

                        dispatch_detail_for_hour["Battery_Charge"] = battery_action_MWh_raw > 0 ? battery_action_MWh_raw : 0;
                        dispatch_detail_for_hour["Battery_Discharge"] = battery_action_MWh_raw < 0 ? -battery_action_MWh_raw : 0;
                        dispatch_detail_for_hour["Battery_End_Charge"] = next_charge_MWh;
                        parent_actions[h + 1][next_charge_idx] = dispatch_detail_for_hour;
                    }
                }
            }
        }

        double min_total_cost = INF_COST;
        int final_charge_idx = -1;
        for (int i = 0; i < BATTERY_CHARGE_LEVELS; ++i) {
            if (dp[HOURS_IN_FORECAST][i] < min_total_cost) {
                min_total_cost = dp[HOURS_IN_FORECAST][i];
                final_charge_idx = i;
            }
        }

        vector<map<string, double>> dispatch_plan(HOURS_IN_FORECAST);
        if (min_total_cost != INF_COST && final_charge_idx != -1) {
            int current_charge_idx = final_charge_idx;
            for (int h = HOURS_IN_FORECAST; h > 0; --h) {
                dispatch_plan[h - 1] = parent_actions[h][current_charge_idx];
                current_charge_idx = parent_charge_idx[h][current_charge_idx];
            }
        } else {
            cout << "No feasible dispatch plan found. Please check generator capacities and battery limits relative to demand.\n";
            return {INF_COST, {}};
        }

        cout << "Energy Dispatch Plan generated using Dynamic Programming.\n";

        // Decrease uptime and flag failed generators
        vector<string> failed_generators;
        double current_total_online_capacity_MW = 0;

        for (auto& gen : generators) {
            bool used = false;

            // Check if generator was used at least once
            for (int h = 0; h < HOURS_IN_FORECAST; ++h) {
                if (dispatch_plan[h].count(gen.name)) {
                    used = true;
                    break;
                }
            }

            if (used && !gen.in_maintenance) {
                gen.uptime_hours_remaining = max(0, gen.uptime_hours_remaining - HOURS_IN_FORECAST);
                
                if (gen.uptime_hours_remaining <= 0) {
                    cout << "⚠ Generator " << gen.name << " has failed due to uptime exhaustion and is now under maintenance.\n";
                    gen.in_maintenance = true;
                    failed_generators.push_back(gen.name);
                }
            }

            if (!gen.in_maintenance) {
                current_total_online_capacity_MW += gen.max_output_MW;
            }
        }

        // Emergency reschedule if we're under capacity now
        double peak_forecast = *max_element(demand_forecast.begin(), demand_forecast.end());
        if (current_total_online_capacity_MW < peak_forecast) {
            emergencyReschedule(peak_forecast);
        }

        return {min_total_cost, dispatch_plan};
    }

    vector<string> scheduleMaintenance(int num_generators_to_maintain, double min_grid_capacity_MW_needed = 0) {
        vector<Generator*> available_gens;
        for (auto& gen : generators) {
            if (!gen.in_maintenance) {
                available_gens.push_back(&gen);
            }
        }

        sort(available_gens.begin(), available_gens.end(), [](Generator* a, Generator* b) {
            return a->uptime_hours_remaining < b->uptime_hours_remaining;
        });

        vector<string> scheduled_for_maintenance;
        double current_total_online_capacity_MW = 0;
        for (const auto& gen : available_gens) {
            current_total_online_capacity_MW += gen->max_output_MW;
        }

        for (int i = 0; i < num_generators_to_maintain && i < available_gens.size(); ++i) {
            Generator* gen_to_maintain = available_gens[i];

            if (current_total_online_capacity_MW - gen_to_maintain->max_output_MW >= min_grid_capacity_MW_needed) {
                gen_to_maintain->in_maintenance = true;
                scheduled_for_maintenance.push_back(gen_to_maintain->name);
                current_total_online_capacity_MW -= gen_to_maintain->max_output_MW;
            } else {
                cout << "Warning: Cannot schedule " << gen_to_maintain->name << " for maintenance (would violate minimum grid capacity " << min_grid_capacity_MW_needed << " MW).\n";
                break;
            }
        }
        cout << "Maintenance Schedule generated using Greedy approach.\n";
        return scheduled_for_maintenance;
    }

    void emergencyReschedule(double min_grid_capacity_MW_needed) {
        double current_capacity = 0;
        for (const auto& gen : generators) {
            if (!gen.in_maintenance) current_capacity += gen.max_output_MW;
        }

        if (current_capacity < min_grid_capacity_MW_needed) {
            cout << "⚠ Emergency: Grid capacity too low! Attempting to restore generators from maintenance...\n";

            // Bring generators back in order of highest capacity first
            vector<Generator*> candidates;
            for (auto& gen : generators) {
                if (gen.in_maintenance && gen.uptime_hours_remaining > 0) {
                    candidates.push_back(&gen);
                }
            }

            sort(candidates.begin(), candidates.end(), [](Generator* a, Generator* b) {
                return a->max_output_MW > b->max_output_MW;
            });

            for (auto* gen : candidates) {
                gen->in_maintenance = false;
                current_capacity += gen->max_output_MW;
                cout << "✅ Restored " << gen->name << " to active duty.\n";

                if (current_capacity >= min_grid_capacity_MW_needed) {
                    cout << "✅ Grid capacity restored to " << current_capacity << " MW\n";
                    break;
                }
            }

            if (current_capacity < min_grid_capacity_MW_needed) {
                cout << "❌ Still under capacity: " << current_capacity << " MW < required " << min_grid_capacity_MW_needed << " MW\n";
            }
        }
    }

    void addGenerator(const string& name, double output, double cost, int uptime) {
        generators.push_back({name, output, cost, uptime, false});
        //cout << "Generator " << name << " added.\n";
    }

    void setBattery(double capacity, double charge_eff, double discharge_eff, double initial_charge) {
        battery = {capacity, charge_eff, discharge_eff, initial_charge};
        battery_initialized = true;
        cout << "Battery set with capacity " << capacity << " MWh.\n";
    }

    void displayComponents() const {
        cout << "\n--- Grid Components ---\n";
        if (battery_initialized) {
            cout << "Battery: Capacity=" << battery.capacity_MWh << " MWh, Charge Eff=" << battery.charge_efficiency * 100 << "%, Discharge Eff=" << battery.discharge_efficiency * 100 << "%, Current Charge=" << battery.current_charge_MWh << " MWh\n";
        } else {
            cout << "Battery: Not initialized.\n";
        }
        cout << "Generators:\n";
        if (generators.empty()) {
            cout << "  No generators added.\n";
        } else {
            for (const auto& gen : generators) {
                cout << "  - " << gen.name << ": Max Output=" << gen.max_output_MW << " MW, Cost=" << gen.cost_per_MWh << " $/MWh, Uptime Remaining=" << gen.uptime_hours_remaining << "h, Status=" << (gen.in_maintenance ? "Maintenance" : "Online") << "\n";
            }
        }
        cout << "-----------------------\n";
    }
};





bool loadfiles(SmartGridManager& grid_manager, vector<double>& loaded_forecast_data) {
    ifstream genFile("generators.txt");
    ifstream batFile("battery.txt");
    ifstream histFile("historical.txt");

    if (!genFile || !batFile || !histFile) {
        cout << "Error: Could not open one or more input files (generators.txt, battery.txt, historical.txt).\n";
        return false;
    }

    // Load generators
    string line;
    while (getline(genFile, line)) {
        if (line.empty() || line[0] == '#') continue; // Skip comments

        istringstream iss(line);
        int id;
        string name;
        double output, cost;
        int uptime;

        if (iss >> id >> name >> output >> cost >> uptime) {
            grid_manager.addGenerator(name, output, cost, uptime);
        }
    }
    cout << "All generators loaded successfully.\n";

    // Load battery
    while (getline(batFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream iss(line);
        double capacity, charge_eff, discharge_eff, initial_charge;
        if (iss >> capacity >> charge_eff >> discharge_eff >> initial_charge) {
            grid_manager.setBattery(capacity, charge_eff, discharge_eff, initial_charge);
        }
    }

    // Load historical demand
    vector<double> historical_data;
    while (getline(histFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        istringstream iss(line);
        double hist_val;
        while (iss >> hist_val) {
            historical_data.push_back(hist_val);
        }
    }

    // Generate forecast using D&C
    loaded_forecast_data = grid_manager.forecastDemand(historical_data);

    return true;
}




// --- Main Function (User Interface) ---
int main() {
    SmartGridManager grid_manager;
    vector<double> loaded_forecast_data;

    if (!loadfiles(grid_manager, loaded_forecast_data)) {
        return 1;
    }

    int choice;
    do {
        cout << "\n--- Smart Grid Energy Management ---\n";
        cout << "1. View Loaded Generators\n";
        cout << "2. View Battery Details\n";
        cout << "3. Display All Grid Components\n";
        cout << "4. Show Forecast (Divide and Conquer)\n";
        cout << "5. Run Dispatch Optimizer (DP)\n";
        cout << "6. Schedule Generator Maintenance (Greedy)\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear buffer

        switch (choice) {

            case 1: {
                cout << "\n--- Loaded Generators ---\n";
                bool any = false;
                for (const auto& gen : grid_manager.generators) {
                    any = true;
                    cout << "  - " << gen.name
                        << ": Max Output = " << gen.max_output_MW << " MW, "
                        << "Cost = $" << gen.cost_per_MWh << "/MWh, "
                        << "Uptime Remaining = " << gen.uptime_hours_remaining << "h, "
                        << "Status = " << (gen.in_maintenance ? "Maintenance" : "Online") << "\n";
                }
                if (!any) {
                    cout << "No generators loaded.\n";
                }
                cout << "--------------------------\n";
                break;
            }
            case 2: {
                cout << "\n--- Loaded Battery ---\n";
                if (grid_manager.battery_initialized) {
                    const auto& b = grid_manager.battery;
                    cout << fixed << setprecision(2);
                    cout << "Capacity = " << b.capacity_MWh << " MWh\n";
                    cout << "Charge Efficiency = " << b.charge_efficiency * 100 << "%\n";
                    cout << "Discharge Efficiency = " << b.discharge_efficiency * 100 << "%\n";
                    cout << "Current Charge = " << b.current_charge_MWh << " MWh\n";
                } else {
                    cout << "Battery not initialized.\n";
                }
                cout << "-----------------------\n";
                break;
            }

            case 3: {
                grid_manager.displayComponents();
                break;
            }

            // Case 4 - Display the 24-hour forecast.
//         - Forecast was generated using D&C from historical.txt in loadfiles().
            case 4: {
                if (loaded_forecast_data.empty()) {
                    cout << "No forecast data available.\n";
                } else {
                    cout << "\n--- 24-Hour Forecast Generated (D&C) ---\n";
                    cout << fixed << setprecision(2);
                    for (int i = 0; i < loaded_forecast_data.size(); ++i) {
                        cout << "Hour " << i + 1 << ": " << loaded_forecast_data[i] << " MWh\n";
                    }
                    cout << "---------------------------------------\n";
                }
                break;
            }
            // Case 5 - Run the Dynamic Programming optimizer using the forecasted demand.
//         - Displays per-hour dispatch decisions and total cost.
            case 5: {
                if (loaded_forecast_data.size() != HOURS_IN_FORECAST) {
                    cout << "Error: Forecast not properly loaded.\n";
                    break;
                }

                auto [total_cost, dispatch_plan] = grid_manager.optimizeEnergyDispatch(loaded_forecast_data);

                // After optimization, check for generator failures
                vector<string> failed_generators;

                for (auto& gen : grid_manager.generators) {
                    if (gen.uptime_hours_remaining <= 0 && !gen.in_maintenance) {
                        gen.in_maintenance = true;
                        failed_generators.push_back(gen.name);
                    }
                }

                if (!failed_generators.empty()) {
                    cout << "\nEMERGENCY: The following generators failed during dispatch:\n";
                    for (const auto& name : failed_generators) {
                        cout << "  - " << name << "\n";
                    }

                    // Optional rescheduling logic
                    cout << "\nAttempting emergency maintenance rescheduling...\n";
                    vector<string> rescheduled = grid_manager.scheduleMaintenance(
                        failed_generators.size(),  // or smarter calculation
                        50.0                       // optional: minimum required capacity
                    );

                    if (!rescheduled.empty()) {
                        cout << "Emergency maintenance scheduled for: ";
                        for (const auto& name : rescheduled) cout << name << " ";
                        cout << "\n";
                    } else {
                        cout << "Could not reschedule failed generators.\n";
                    }
                }

                if (total_cost != INF_COST) {
                    cout << "\n--- Optimal Energy Dispatch Plan ---\n";
                    cout << fixed << setprecision(2);
                    cout << "Total Estimated Cost: $" << total_cost << "\n";
                    for (int h = 0; h < HOURS_IN_FORECAST; ++h) {
                        cout << "Hour " << h + 1 << " (Demand: " << loaded_forecast_data[h] << " MWh):\n";
                        for (const auto& entry : dispatch_plan[h]) {
                            if (entry.first == "Battery_End_Charge") {
                                cout << "  Battery End Charge: " << entry.second << " MWh\n";
                            } else {
                                cout << "  " << entry.first << ": " << entry.second << " MWh\n";
                            }
                        }
                    }
                    cout << "------------------------------------\n";
                } else {
                    cout << "Could not find a feasible dispatch plan. Check capacity/demand.\n";
                }
                break;
            }
            // Case 6 - Greedy algorithm to schedule maintenance.
//         - Only generators not in maintenance and with enough capacity are selected.
            case 6: {
                int num_to_maintain;
                double min_capacity;
                cout << "Enter number of generators to schedule for maintenance: ";
                cin >> num_to_maintain;
                cout << "Enter minimum required grid capacity during maintenance (MW): ";
                cin >> min_capacity;
                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                vector<string> scheduled_gens = grid_manager.scheduleMaintenance(num_to_maintain, min_capacity);

                cout << "\n--- Maintenance Schedule ---\n";
                if (scheduled_gens.empty() && num_to_maintain > 0) {
                    cout << "No generators could be scheduled for maintenance under the given constraints.\n";
                } else if (scheduled_gens.empty()) {
                    cout << "No generators were requested for maintenance.\n";
                }
                else {
                    cout << "Generators scheduled: ";
                    for (const auto& name : scheduled_gens) {
                        cout << name << " ";
                    }
                    cout << "\n";
                }
                cout << "--------------------------\n";
                break;
            }
            case 0:
                cout << "Exiting Smart Grid Energy Management. Goodbye!\n";
                break;
            default:
                cout << "Invalid choice. Please try again.\n";
                break;
        }

    } while (choice != 0);

    return 0;
}
