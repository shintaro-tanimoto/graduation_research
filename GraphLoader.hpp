#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include "MakeBaseGraph.hpp" // CoreGraph, ConnectionRule, Point3D を使うため

// 読み込み状態を管理するための列挙型
enum class ParseState {
    None,
    ReadingCore,
    ReadingRules
};

/**
 * @brief 1つの定義ファイルからコアグラフと接続ルールの両方を読み込みます。
 */
inline void loadDefinitions(const std::string& filename, CoreGraph& core_graph, std::vector<ConnectionRule>& rules) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open definition file: " + filename);
    }

    // 古いデータをクリア
    core_graph = CoreGraph();
    rules.clear();

    std::string line, keyword;
    ParseState state = ParseState::None;
    ConnectionRule current_rule;
    bool in_rule = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // 空行やコメントはスキップ
        
        std::stringstream ss(line);
        ss >> keyword;

        if (keyword == "CORE_GRAPH") {
            state = ParseState::ReadingCore;
            continue;
        } else if (keyword == "RULES") {
            state = ParseState::ReadingRules;
            continue;
        }

        switch (state) {
            case ParseState::ReadingCore: {
                std::string v1 = keyword; // 最初の単語はkeywordに入っている
                std::string v2;
                ss >> v2;
                if (!v2.empty()) {
                    core_graph.addEdge(v1, v2);
                }
                break;
            }
            case ParseState::ReadingRules: {
                if (keyword == "RULE") {
                    if (in_rule) rules.push_back(current_rule);
                    current_rule = ConnectionRule();
                    in_rule = true;
                } else if (keyword == "VECTOR") {
                    ss >> current_rule.vector.x >> current_rule.vector.y >> current_rule.vector.z;
                } else if (keyword == "CONNECT") {
                    std::string v1, v2;
                    ss >> v1 >> v2;
                    current_rule.connections.push_back({v1, v2});
                }
                break;
            }
            default:
                // CORE_GRAPH や RULES が見つかるまでは何もしない
                break;
        }
    }
    
    if (in_rule) {
        rules.push_back(current_rule); // 最後のルールを追加
    }
    
    core_graph.update();
}

#endif // GRAPH_LOADER_HPP