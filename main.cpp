#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

int sim(std::string guess, const std::string &ans) {
    int result_green = 0;
    int result_yellow = 0;
    std::vector<int> hist(26, 0);
    for (char c : ans) {
        hist[c - 'a']++;
    }
    for (int i = 0; i < 5; i++) {
        result_green *= 3;
        if (guess[i] == ans[i]) {
            result_green += 1;
            hist[guess[i] - 'a']--;
            guess[i] = '!';
        }
    }
    for (int i = 0; i < 5; i++) {
        result_yellow *= 3;
        if (guess[i] == '!') continue;
        if (hist[guess[i] - 'a'] > 0) {
            result_yellow += 2;
            hist[guess[i] - 'a']--;
        }
    }
    return result_green + result_yellow;
}

std::string res_str(int res) {
    std::string s;
    for (int i = 0; i < 5; i++) {
        s += "bgy"[res % 3];
        res /= 3;
    }
    std::reverse(s.begin(), s.end());
    return s;
}

int res_int(const std::string &res) {
    int result = 0;
    for (int i = 0; i < 5; i++) {
        result *= 3;
        if (res[i] == 'y') {
            result += 2;
        } else if (res[i] == 'g') {
            result += 1;
        }
    }
    return result;
}

std::vector<std::string> load(const std::string &filename) {
    std::ifstream fin(filename);
    std::vector<std::string> out;
    while (fin) {
        std::string s;
        fin >> s;
        if (s.length() == 5) {
            out.push_back(s);
        }
    }
    return out;
}

struct row {
    std::string guess;
    std::string res;
};

struct rowi {
    int guess;
    int res;
};

int letters(const std::string &s) {
    std::vector<int> hist(26, 0);
    for (char c : s) {
        hist[c - 'a']++;
    }
    int n = 0;
    for (int k : hist) {
        if (k > 0) {
            n++;
        }
    }
    return n;
}

class solver {
    friend class wordle;
    int sims(const int guess, const int ans) {
        int key = (guess << 15) + ans;
        if (sim_result[key] == -1) {
            sim_result[key] = sim(guesses[guess], answers[ans]);
        }
        return sim_result[key];
    }
    std::vector<int> sim_result;

    std::vector<rowi> from_row(const std::vector<row> &rs) const {
        std::vector<rowi> rsi;
        for (const auto &r : rs) {
            const int g_ind = guesses_ind.at(r.guess);
            const int res = res_int(r.res);
            rsi.push_back({g_ind, res});
        }
        return rsi;
    }

   public:
    std::unordered_map<std::string, int> answers_ind;
    std::unordered_map<std::string, int> guesses_ind;
    std::vector<std::string> answers;
    std::vector<std::string> guesses;
    std::vector<int> guess_letters;
    std::vector<int> answer_letters;

    std::vector<int> solve_s(const std::vector<row> &rs) {
        return solve(from_row(rs), all_valid());
    }

    std::vector<int> all_valid() const {
        std::vector<int> valid;
        for (int i = 0; i < answers.size(); i++) {
            valid.push_back(i);
        }
        return valid;
    }

    std::vector<int> solve(const std::vector<rowi> &rs,
                           std::vector<int> valid) {
        for (const auto &r : rs) {
            std::vector<int> valid2;
            for (const int i : valid) {
                if (sims(r.guess, i) == r.res) {
                    valid2.push_back(i);
                }
            }
            valid = std::move(valid2);
        }
        std::vector<int> out;
        out.insert(out.end(), valid.begin(), valid.end());
        return out;
    }

    solver(std::vector<std::string> answers, std::vector<std::string> guesses)
        : answers{answers}, guesses{guesses} {
        answer_letters.resize(answers.size());
        guess_letters.resize(guesses.size());
        for (int i = 0; i < answers.size(); i++) {
            answers_ind[answers[i]] = i;
            answer_letters[i] = letters(answers[i]);
        }
        for (int i = 0; i < guesses.size(); i++) {
            guesses_ind[guesses[i]] = i;
            guess_letters[i] = letters(guesses[i]);
        }
        sim_result.resize(guesses.size() << 15);
        std::fill(sim_result.begin(), sim_result.end(), -1);
        /*
        for (int i = 0; i < guesses.size(); i++) {
            for (int j = 0; j < answers.size(); j++) {
                sim_result[(i << 15) + j] = sim(guesses[i], answers[j]);
            }
        }
        */
    }
};

struct minimax_res {
    int depth;
    int population;
    int play;
};

class wordle {
   public:
    void survivle_s(const std::vector<row> &g, const std::string ans) {
        longest_g.clear();
        auto gi = guess_solver.from_row(g);
        survivle(gi, guess_solver.answers_ind.at(ans));
    }

    void survivle(std::vector<rowi> &g, const int ans) {
        if (g.size() > 0 && g[g.size() - 1].guess == ans) {
            if (g.size() > longest_g.size()) {
                longest_g = g;
                std::cout << g.size() << ": ";
                for (const auto &r : longest_g) {
                    std::cout << guess_solver.answers[r.guess] << " ";
                }
                std::cout << std::endl;
            }
            return;
        }
        auto nex = guess_solver.solve(g, guess_solver.all_valid());
        std::sort(nex.begin(), nex.end(), [&](int x, int y) {
            return guess_solver.answer_letters[x] <
                   guess_solver.answer_letters[y];
        });

        for (const int i : nex) {
            g.push_back({i, guess_solver.sims(i, ans)});
            survivle(g, ans);
            g.pop_back();
        }
    }
    minimax_res wordle_solve(std::vector<rowi> &g, const int depth,
                             const std::vector<int> &valid,
                             const int max_depth) {
        std::vector<int> nex;
        if (depth > 0) {
            nex = answer_solver.solve({g[g.size() - 1]}, valid);
        } else {
            nex = valid;
        }
        if (depth == max_depth) {
            return {max_depth, static_cast<int>(nex.size()), 0};
        }
        if (nex.size() == 1) {
            return {depth, 1, nex[0]};
        }
        minimax_res best{99999, 99999, 0};
        for (int i = 0; i < answer_solver.guesses.size(); i++) {
            minimax_res worst{0, 0, 0};
            std::unordered_set<int> patterns;
            for (const int ans : nex) {
                patterns.insert(answer_solver.sims(i, ans));
            }

            for (const int pattern : patterns) {
                g.push_back({i, pattern});
                minimax_res x = wordle_solve(g, depth + 1, nex,
                                             std::min(max_depth, best.depth));
                if (x.depth > worst.depth ||
                    (x.depth == worst.depth &&
                     x.population > worst.population)) {
                    worst = x;
                }
                g.pop_back();
            }
            if (worst.depth < best.depth ||
                (worst.depth == best.depth &&
                 worst.population < best.population)) {
                best = worst;
                best.play = i;
                if (depth == 0) {
                    std::cerr << best.depth << ", " << answer_solver.guesses[i]
                              << ", " << best.population << std::endl;
                }
            }
        }
        return best;
    }
    minimax_res wordle_solve_s(const std::vector<row> &g) {
        auto gi = answer_solver.from_row(g);
        auto nex = answer_solver.solve(gi, answer_solver.all_valid());
        return wordle_solve(gi, 0, nex, 2);
    }

    void opener() {
        int best = 99999;
        int besti = 0;
        int bestj = 0;
        std::vector<int> x;
        for (int i = 0; i < answer_solver.guesses.size(); i++) {
            x.push_back(i);
        }
        std::sort(x.begin(), x.end(), [&](int i, int j) {
            return answer_solver.guess_letters[i] >
                   answer_solver.guess_letters[j];
        });
        // for (int xi = 0; xi < answer_solver.guesses.size(); xi++) {
        int xi = answer_solver.guesses_ind["crane"];
        {
            for (int xj = 0; xj < xi; xj++) {
                for (int xk = 0; xk < xj; xk++) {
                    // const int i = x[xi];
                    const int i = xi;
                    const int j = x[xj];
                    const int k = x[xk];
                    int worst = 0;
                    for (int a = 0; a < answer_solver.answers.size(); a++) {
                        const auto s1 = answer_solver.sims(i, a);
                        const auto s2 = answer_solver.sims(j, a);
                        const auto s3 = answer_solver.sims(k, a);
                        const auto w =
                            answer_solver.solve({{i, s1}, {j, s2}, {k, s3}},
                                                answer_solver.all_valid());
                        if (w.size() > worst) {
                            worst = w.size();
                            if (worst >= best) {
                                break;
                            }
                        }
                    }
                    if (worst < best) {
                        besti = i;
                        bestj = j;
                        best = worst;
                        std::cout << answer_solver.guesses[i] << ", "
                                  << answer_solver.guesses[j] << ", "
                                  << answer_solver.guesses[k] << ": " << best
                                  << std::endl;
                    }
                }
            }
        }
    }

    solver guess_solver;
    solver answer_solver;
    std::vector<rowi> longest_g;

    wordle(std::vector<std::string> answers, std::vector<std::string> guesses)
        : answer_solver(answers, guesses), guess_solver(guesses, guesses) {}
};

int main() {
    auto answers{load("answers.txt")};
    auto guesses{load("guesses.txt")};
    guesses.insert(guesses.end(), answers.begin(), answers.end());
    std::sort(answers.begin(), answers.end());
    // std::sort(guesses.begin(), guesses.end());

    /*
    std::sort(
        guesses.begin(), guesses.end(),
        [&](std::string i, std::string j) { return letters(i) > letters(j); });
        */
    wordle w(answers, guesses);
    /*
    w.survivle_s({}, "cocco");
    */
    // w.opener();
    auto res = w.wordle_solve_s(
        {{"crane", "bbbyy"}, {"bolts", "bbbby"}, {"wimpy", "bbbbb"}});
    std::cerr << res.depth << ", " << w.answer_solver.guesses[res.play] << ", "
              << res.population << std::endl;
    const auto solution = w.answer_solver.solve_s(
        {{"crane", "bbbyy"}, {"bolts", "bbbby"}, {"wimpy", "bbbbb"}});
    for (const auto &s : solution) {
        std::cout << w.answer_solver.answers[s] << std::endl;
    }
}
