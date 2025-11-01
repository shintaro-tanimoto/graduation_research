#ifndef GRAPH_LOADER_HPP
#define GRAPH_LOADER_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "1_core_graph/MakeBaseGraph.hpp" 
#include "3_geometry/ObjTypes.hpp" // ObjMesh 構造体のため

// 読み込み状態を管理するための列挙型
enum class ParseState {
    None,
    ReadingCore,
    ReadingRules,
    ReadingMesh 
};

/**
 * @brief 1つの定義ファイルからコアグラフ、ルール、メッシュを読み込みます。
 */
inline void loadDefinitions(
    const std::string& filename, 
    CoreGraph& core_graph, 
    std::vector<ConnectionRule>& rules,
    std::map<std::string, ObjMesh>& mesh_data
) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error: Cannot open definition file: " + filename);
    }

    // 古いデータをクリア
    core_graph = CoreGraph();
    rules.clear();
    mesh_data.clear();

    std::string line, keyword;
    ParseState state = ParseState::None;
    ConnectionRule current_rule;
    bool in_rule = false;
    
    std::string current_mesh_type = ""; 

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; 
        
        std::stringstream ss(line);
        ss >> keyword;

        if (keyword == "CORE_GRAPH") {
            state = ParseState::ReadingCore;
            current_mesh_type = ""; 
            continue;
        } else if (keyword == "RULES") {
            state = ParseState::ReadingRules;
            current_mesh_type = ""; 
            continue;
        } else if (keyword == "VERTEX_MESH") { 
            state = ParseState::ReadingMesh;
            ss >> current_mesh_type; 
            if (!current_mesh_type.empty()) {
                mesh_data[current_mesh_type] = ObjMesh(); 
            }
            continue;
        }

        switch (state) {
            // --- ▼ 修正点: vertexId() 呼び出しを削除 ▼ ---
            case ParseState::ReadingCore: {
                std::string v1 = keyword; 
                std::string v2;
                ss >> v2;

                if (!v2.empty()) {
                    // addEdgeだけで、tdzdd::Graphは頂点を認識します
                    core_graph.addEdge(v1, v2);
                }
                break;
            }
            // --- ▲ 修正点 ▲ ---
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
            case ParseState::ReadingMesh: {
                if (current_mesh_type.empty()) break; 

                ObjMesh& current_mesh = mesh_data.at(current_mesh_type);

                if (keyword == "v") {
                    Point3D p;
                    ss >> p.x >> p.y >> p.z;
                    current_mesh.vertices.push_back(p);
                } else if (keyword == "f") {
                    std::vector<int> face_indices;
                    int idx;
                    while (ss >> idx) {
                        face_indices.push_back(idx - 1); // 0-based
                    }
                    if (face_indices.size() >= 3) {
                        current_mesh.faces.push_back(face_indices);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    
    if (in_rule) {
        rules.push_back(current_rule); // 最後のルールを追加
    }

    core_graph.update();
}
#endif // GRAPH_LOADER_HPP