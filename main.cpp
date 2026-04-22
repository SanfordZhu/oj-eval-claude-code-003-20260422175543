#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
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
};

struct Team {
    string name;
    map<string, ProblemState> problems;
    vector<Submission> submissions;
    int solved_count = 0;
    int total_penalty = 0;
    vector<int> solve_times;
};

class ICPCSystem {
private:
    map<string, Team> teams;
    bool competition_started = false;
    int duration_time = 0;
    int problem_count = 0;
    bool frozen = false;
    bool scoreboard_flushed = false;
    vector<string> ranking_order;

    int getPenalty(const ProblemState& ps) {
        if (!ps.solved) return 0;
        return 20 * ps.total_incorrect + ps.solve_time;
    }

    void updateTeamStats(Team& team) {
        team.solved_count = 0;
        team.total_penalty = 0;
        team.solve_times.clear();
        for (const auto& p : team.problems) {
            if (p.second.solved) {
                team.solved_count++;
                team.total_penalty += getPenalty(p.second);
                team.solve_times.push_back(p.second.solve_time);
            }
        }
        sort(team.solve_times.rbegin(), team.solve_times.rend());
    }

    bool compareTeams(const string& a, const string& b) {
        const Team& ta = teams[a];
        const Team& tb = teams[b];
        if (ta.solved_count != tb.solved_count)
            return ta.solved_count > tb.solved_count;
        if (ta.total_penalty != tb.total_penalty)
            return ta.total_penalty < tb.total_penalty;
        for (size_t i = 0; i < max(ta.solve_times.size(), tb.solve_times.size()); i++) {
            int ta_time = (i < ta.solve_times.size()) ? ta.solve_times[i] : 0;
            int tb_time = (i < tb.solve_times.size()) ? tb.solve_times[i] : 0;
            if (ta_time != tb_time)
                return ta_time < tb_time;
        }
        return a < b;
    }

    void updateRanking() {
        ranking_order.clear();
        for (const auto& t : teams) {
            ranking_order.push_back(t.first);
        }
        if (scoreboard_flushed) {
            sort(ranking_order.begin(), ranking_order.end(),
                [this](const string& a, const string& b) { return compareTeams(a, b); });
        } else {
            sort(ranking_order.begin(), ranking_order.end());
        }
    }

public:
    void addTeam(const string& name) {
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return;
        }
        if (teams.count(name)) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return;
        }
        teams[name].name = name;
        updateRanking();
        cout << "[Info]Add successfully." << endl;
    }

    void startCompetition(int duration, int problems) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started." << endl;
            return;
        }
        competition_started = true;
        duration_time = duration;
        problem_count = problems;
        cout << "[Info]Competition starts." << endl;
    }

    void submit(const string& problem, const string& team_name, const string& status, int time) {
        if (!teams.count(team_name)) return;
        Team& team = teams[team_name];
        team.submissions.push_back({problem, status, time});

        ProblemState& ps = team.problems[problem];
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
                updateTeamStats(team);
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
        for (auto& t : teams) {
            for (auto& p : t.second.problems) {
                if (!p.second.solved) {
                    p.second.frozen = true;
                } else {
                    p.second.solved_before_freeze = true;
                }
            }
        }
        cout << "[Info]Freeze scoreboard." << endl;
    }

    string getProblemDisplay(const Team& team, const string& problem) {
        auto it = team.problems.find(problem);
        if (it == team.problems.end()) {
            return ".";
        }
        const ProblemState& ps = it->second;
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
            const string& name = ranking_order[i];
            const Team& team = teams[name];
            cout << name << " " << (i + 1) << " " << team.solved_count << " " << team.total_penalty;
            for (int p = 0; p < problem_count; p++) {
                string prob = string(1, 'A' + p);
                cout << " " << getProblemDisplay(team, prob);
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
            string team_to_unfreeze = "";
            string problem_to_unfreeze = "";
            int lowest_rank = -1;

            for (size_t i = 0; i < ranking_order.size(); i++) {
                const string& name = ranking_order[i];
                const Team& team = teams[name];
                for (int p = 0; p < problem_count; p++) {
                    string prob = string(1, 'A' + p);
                    auto it = team.problems.find(prob);
                    if (it != team.problems.end() && it->second.frozen && it->second.submissions_after_freeze > 0) {
                        if ((int)i > lowest_rank) {
                            lowest_rank = i;
                            team_to_unfreeze = name;
                            problem_to_unfreeze = prob;
                        }
                        break;
                    }
                }
                if (team_to_unfreeze != "") break;
            }

            if (team_to_unfreeze == "") break;

            Team& team = teams[team_to_unfreeze];
            ProblemState& ps = team.problems[problem_to_unfreeze];

            vector<string> old_ranking = ranking_order;
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
                    string displaced = old_ranking[new_rank];
                    cout << team_to_unfreeze << " " << displaced << " " << team.solved_count << " " << team.total_penalty << endl;
                }
            }
        }

        printScoreboard();
        frozen = false;
    }

    void queryRanking(const string& team_name) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }
        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }
        int rank = -1;
        for (size_t i = 0; i < ranking_order.size(); i++) {
            if (ranking_order[i] == team_name) {
                rank = i + 1;
                break;
            }
        }
        cout << "[" << team_name << "] NOW AT RANKING " << rank << endl;
    }

    void querySubmission(const string& team_name, const string& problem, const string& status) {
        if (!teams.count(team_name)) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }
        cout << "[Info]Complete query submission." << endl;
        const Team& team = teams[team_name];
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
