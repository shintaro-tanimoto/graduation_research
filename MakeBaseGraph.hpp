#ifndef MAKE_BASE_GRAPH_HPP
#define MAKE_BASE_GRAPH_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <iostream>
#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/Graph.hpp>

// 座標をdouble型で扱えるように変更
struct Point3D {
    double x, y, z;
    bool operator<(const Point3D& other) const { return std::tie(x, y, z) < std::tie(other.x, other.y, other.z); }
    Point3D operator+(const Point3D& other) const { return {x + other.x, y + other.y, z + other.z}; }
};

using CoreGraph = tdzdd::Graph;

struct ConnectionRule {
    Point3D vector;
    std::vector<std::pair<std::string, std::string>> connections;
};

// GraphData構造体を拡張
struct GraphData {
    tdzdd::Graph full_graph; // 全体の詳細なグラフ
    std::map<int, Point3D> core_locations; // コアIDからその座標へのマップ
    // どのコアIDのペアが接続されているかを記録 (重複なし)
    std::set<std::pair<int, int>> core_connectivity;
};

/**
 * @brief コアグラフと接続ルールに基づき、複合的な3次元グラフとその骨格情報を生成します。
 */
inline GraphData make_base_graph(
    const CoreGraph& core_graph,
    const std::vector<ConnectionRule>& rules,
    int n)
{
    GraphData data;
    std::map<Point3D, int> coord_to_core_id;
    std::vector<Point3D> frontier;
    int next_core_id = 0;

    Point3D origin = {0, 0, 0};
    int origin_core_id = next_core_id++;
    
    coord_to_core_id[origin] = origin_core_id;
    frontier.push_back(origin);
    data.core_locations[origin_core_id] = origin;

    // 詳細グラフに最初のコアを追加
    for (int i = 0; i < core_graph.edgeSize(); ++i) {
        const auto& edge = core_graph.edgeInfo(i);
        std::string u_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v1);
        std::string v_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v2);
        data.full_graph.addEdge(u_name, v_name);
    }
    // ▼▼▼ ログ出力 ▼▼▼
    std::cerr << "Placed core " << origin_core_id << " at (0, 0, 0)" << std::endl;

    for (int i = 0; i < n; ++i) {
        std::vector<Point3D> next_frontier;
        for (const auto& current_coord : frontier) {
            int current_core_id = coord_to_core_id.at(current_coord);

            for (const auto& rule : rules) {
                Point3D next_coord = current_coord + rule.vector;
                int destination_core_id;

                if (coord_to_core_id.count(next_coord)) {
                    destination_core_id = coord_to_core_id.at(next_coord);
                } else {
                    destination_core_id = next_core_id++;
                    coord_to_core_id[next_coord] = destination_core_id;
                    next_frontier.push_back(next_coord);
                    data.core_locations[destination_core_id] = next_coord;

                    for (int j = 0; j < core_graph.edgeSize(); ++j) {
                        const auto& edge = core_graph.edgeInfo(j);
                        std::string u_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v1);
                        std::string v_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v2);
                        data.full_graph.addEdge(u_name, v_name);
                    }
                    // ▼▼▼ ログ出力 ▼▼▼
                    std::cerr << "Placed core " << destination_core_id << " at (" << next_coord.x << ", " << next_coord.y << ", " << next_coord.z << ")" << std::endl;
                }
                
                int id1 = std::min(current_core_id, destination_core_id);
                int id2 = std::max(current_core_id, destination_core_id);
                if (id1 != id2) {
                     data.core_connectivity.insert({id1, id2});
                }

                // 詳細グラフの辺を追加
                for (const auto& conn : rule.connections) {
                    std::string u_name = std::to_string(current_core_id) + "_" + conn.first;
                    std::string v_name = std::to_string(destination_core_id) + "_" + conn.second;
                    data.full_graph.addEdge(u_name, v_name);
                    
                    // ▼▼▼ 【復活】詳細な接続ログを出力 ▼▼▼
                    std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
                }
            }
        }
        frontier = next_frontier;
        if (frontier.empty()) break;
    }

    data.full_graph.update();
    return data;
}

#endif // MAKE_BASE_GRAPH_HPP