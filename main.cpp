#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemState {
    int incorrect_before_freeze = 0;
    int submissions_after_freeze = 0;
    bool solved = false;
    int solve_time = 0;
    int total_incorrect = 0;
    bool frozen = false;
    bool solved_before_freeze = false;
    bool has_submission_before_freeze = false;
};

struct Team {
    string name;
    ProblemState problems[26];
    vector<Submission> submissions;
    int solved_count = 0;
    int total_penalty = 0;
    vector<int> solve_times;
};

class ICPCSystem {
private:
    vector<Team> teams;
    map<string, int> team_index;
    bool competition_started = false;
    int problem_count = 0;
    bool frozen = false;
    bool scoreboard_flushed = false;
    vector<int> ranking_order;

    int getProblemIndex(const string& problem) {
        return problem[0] - 'A';
    }

    int getPenalty(const ProblemState& ps) {
        if (!ps.solved) return 0;
        return 20 * ps.total_incorrect + ps.solve_time;
    }

    void updateTeamStats(int team_idx) {
        Team& team = teams[team_idx];
        team.solved_count = 0;
        team.total_penalty = 0;
        team.solve_times.clear();
        for (int i = 0; i < problem_count; i++) {
            const ProblemState& ps = team.problems[i];
            if (ps.solved) {
                team.solved_count++;
                team.total_penalty += getPenalty(ps);
                team.solve_times.push_back(ps.solve_time);
            }
        }
        sort(team.solve_times.rbegin(), team.solve_times.rend());
    }

    void updateRanking() {
        ranking_order.clear();
        ranking_order.reserve(teams.size());
        for (size_t i = 0; i < teams.size(); i++) {
            ranking_order.push_back(i);
        }
        if (scoreboard_flushed) {
            sort(ranking_order.begin(), ranking_order.end(),
                [this](int a, int b) {
                    const Team& ta = teams[a];
                    const Team& tb = teams[b];
                    if (ta.solved_count != tb.solved_count)
                        return ta.solved_count > tb.solved_count;
                    if (ta.total_penalty != tb.total_penalty)
                        return ta.total_penalty < tb.total_penalty;
                    size_t max_len = max(ta.solve_times.size(), tb.solve_times.size());
                    for (size_t i = 0; i < max_len; i++) {
                        int ta_time = (i < ta.solve_times.size()) ? ta.solve_times[i] : 0;
                        int tb_time = (i < tb.solve_times.size()) ? tb.solve_times[i] : 0;
                        if (ta_time != tb_time)
                            return ta_time < tb_time;
                    }
                    return ta.name < tb.name;
                });
        } else {
            sort(ranking_order.begin(), ranking_order.end(),
                [this](int a, int b) { return teams[a].name < teams[b].name; });
        }
    }

public:
    void addTeam(const string& name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return;
        }
        if (team_index.count(name)) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return;
        }
        int idx = teams.size();
        Team t;
        t.name = name;
        teams.push_back(t);
        team_index[name] = idx;
        updateRanking();
        cout << "[Info]Add successfully." << endl;
    }

    void startCompetition(int /*duration*/, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started." << endl;
            return;
        }
        competition_started = true;
        problem_count = problems;
        cout << "[Info]Competition starts." << endl;
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        auto it = team_index.find(team_name);
        if (it == team_index.end()) return;
        int team_idx = it->second;
        Team& team = teams[team_idx];
        team.submissions.push_back({problem, status, time});

        int prob_idx = getProblemIndex(problem);
        ProblemState& ps = team.problems[prob_idx];

        if (!frozen) {
            ps.has_submission_before_freeze = true;
        }

        if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solve_time = time;
                if (frozen) {
                    ps.submissions_after_freeze++;
                    ps.frozen = true;
                } else {
                    ps.incorrect_before_freeze = ps.total_incorrect;
                }
                updateTeamStats(team_idx);
            } else {
                ps.total_incorrect++;
                if (!frozen) {
                    ps.incorrect_before_freeze = ps.total_incorrect;
                } else {
                    ps.submissions_after_freeze++;
                    ps.frozen = true;
                }
            }
        } else {
            if (frozen) {
                ps.submissions_after_freeze++;
            }
        }
    }

    void flushScoreboard() {
        scoreboard_flushed = true;
        updateRanking();
        cout << "[Info]Flush scoreboard." << endl;
    }

    void freezeScoreboard() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            return;
        }
        frozen = true;
        for (auto& team : teams) {
            for (int i = 0; i < problem_count; i++) {
                ProblemState& ps = team.problems[i];
                ps.frozen = false;
                ps.submissions_after_freeze = 0;
                if (!ps.solved && ps.has_submission_before_freeze) {
                    ps.frozen = true;
                } else if (ps.solved) {
                    ps.solved_before_freeze = true;
                }
            }
        }
        cout << "[Info]Freeze scoreboard." << endl;
    }

    string getProblemDisplay(const Team& team, int prob_idx) {
        const ProblemState& ps = team.problems[prob_idx];
        if (ps.frozen) {
            if (ps.incorrect_before_freeze == 0) {
                return "0/" + to_string(ps.submissions_after_freeze);
            }
            return "-" + to_string(ps.incorrect_before_freeze) + "/" + to_string(ps.submissions_after_freeze);
        }
        if (ps.solved) {
            if (ps.total_incorrect == 0) return "+";
            return "+" + to_string(ps.total_incorrect);
        }
        if (ps.total_incorrect == 0) return ".";
        return "-" + to_string(ps.total_incorrect);
    }

    void printScoreboard() {
        for (size_t i = 0; i < ranking_order.size(); i++) {
            int idx = ranking_order[i];
            const Team& team = teams[idx];
            cout << team.name << " " << (i + 1) << " " << team.solved_count << " " << team.total_penalty;
            for (int p = 0; p < problem_count; p++) {
                cout << " " << getProblemDisplay(team, p);
            }
            cout << endl;
        }
    }

    void scrollScoreboard() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
            return;
        }
        cout << "[Info]Scroll scoreboard." << endl;
        flushScoreboard();
        printScoreboard();

        while (true) {
            int team_to_unfreeze = -1;
            int problem_to_unfreeze = -1;
            int lowest_rank = -1;

            for (size_t i = 0; i < ranking_order.size(); i++) {
                int idx = ranking_order[i];
                const Team& team = teams[idx];
                for (int p = 0; p < problem_count; p++) {
                    const ProblemState& ps = team.problems[p];
                    if (ps.frozen && ps.submissions_after_freeze > 0) {
                        if ((int)i > lowest_rank) {
                            lowest_rank = i;
                            team_to_unfreeze = idx;
                            problem_to_unfreeze = p;
                        }
                        break;
                    }
                }
                if (team_to_unfreeze != -1) break;
            }

            if (team_to_unfreeze == -1) break;

            Team& team = teams[team_to_unfreeze];
            ProblemState& ps = team.problems[problem_to_unfreeze];

            vector<int> old_ranking = ranking_order;
            ps.frozen = false;
            updateRanking();

            if (ranking_order != old_ranking) {
                int new_rank = -1;
                for (size_t i = 0; i < ranking_order.size(); i++) {
                    if (ranking_order[i] == team_to_unfreeze) {
                        new_rank = i;
                        break;
                    }
                }
                if (new_rank >= 0 && (size_t)new_rank < old_ranking.size()) {
                    int displaced = old_ranking[new_rank];
                    cout << team.name << " " << teams[displaced].name << " " << team.solved_count << " " << team.total_penalty << endl;
                }
            }
        }

        printScoreboard();
        frozen = false;
    }

    void queryRanking(const string& team_name) {
        auto it = team_index.find(team_name);
        if (it == team_index.end()) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }
        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }
        int rank = -1;
        for (size_t i = 0; i < ranking_order.size(); i++) {
            if (ranking_order[i] == it->second) {
                rank = i + 1;
                break;
            }
        }
        cout << "[" << team_name << "] NOW AT RANKING " << rank << endl;
    }

    void querySubmission(const string& team_name, const string& problem, const string& status) {
        auto it = team_index.find(team_name);
        if (it == team_index.end()) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }
        cout << "[Info]Complete query submission." << endl;
        const Team& team = teams[it->second];
        Submission result;
        bool found = false;
        for (const auto& sub : team.submissions) {
            bool match_problem = (problem == "ALL" || sub.problem == problem);
            bool match_status = (status == "ALL" || sub.status == status);
            if (match_problem && match_status) {
                result = sub;
                found = true;
            }
        }
        if (!found) {
            cout << "Cannot find any submission." << endl;
        } else {
            cout << "[" << team_name << "] " << result.problem << " " << result.status << " " << result.time << endl;
        }
    }

    void endCompetition() {
        cout << "[Info]Competition ends." << endl;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string d, p;
            int duration, problems;
            iss >> d >> duration >> p >> problems;
            system.startCompetition(duration, problems);
        } else if (cmd == "SUBMIT") {
            string problem, by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            system.submit(problem, team_name, status, time);
        } else if (cmd == "FLUSH") {
            system.flushScoreboard();
        } else if (cmd == "FREEZE") {
            system.freezeScoreboard();
        } else if (cmd == "SCROLL") {
            system.scrollScoreboard();
        } else if (cmd == "QUERY_RANKING") {
            string team_name;
            iss >> team_name;
            system.queryRanking(team_name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, p, eq_problem, and_str, s, eq_status;
            iss >> team_name >> where >> p >> eq_problem >> and_str >> s >> eq_status;
            string problem = eq_problem.substr(eq_problem.find('=') + 1);
            string status = eq_status.substr(eq_status.find('=') + 1);
            system.querySubmission(team_name, problem, status);
        } else if (cmd == "END") {
            system.endCompetition();
            break;
        }
    }

    return 0;
}
