#ifndef MAKE_BASE_GRAPH_HPP
#define MAKE_BASE_GRAPH_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <cmath> 
#include <ostream> // <-- 【追加】 std::ostream のため
#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/Graph.hpp>

// 座標をdouble型で扱う
struct Point3D {
    double x, y, z;
    bool operator<(const Point3D& other) const {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }
    Point3D operator+(const Point3D& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
};

// 【新設】std::mapのキーとして使う、整数化されたグリッド座標
struct GridPoint3D {
    long long x_grid, y_grid, z_grid; // 整数型にする
    bool operator<(const GridPoint3D& other) const {
        return std::tie(x_grid, y_grid, z_grid) < std::tie(other.x_grid, other.y_grid, other.z_grid);
    }
};

// 【新設】double座標をグリッド座標に変換するヘルパー関数
const double TOLERANCE = 0.1; // 許容誤差
const double QUANTIZATION_FACTOR = 1.0 / TOLERANCE; // 量子化係数 (0.1なら10)

inline GridPoint3D quantize(const Point3D& p) {
    return {
        static_cast<long long>(std::round(p.x * QUANTIZATION_FACTOR)),
        static_cast<long long>(std::round(p.y * QUANTIZATION_FACTOR)),
        static_cast<long long>(std::round(p.z * QUANTIZATION_FACTOR))
    };
}

using CoreGraph = tdzdd::Graph;

struct ConnectionRule {
    Point3D vector;
    std::vector<std::pair<std::string, std::string>> connections;
};

struct GraphData {
    tdzdd::Graph full_graph;
    std::map<int, Point3D> core_locations; // コアの座標はdoubleで保持
    std::set<std::pair<int, int>> core_connectivity;
};

// --- ▼ 【修正】 関数シグネチャと std::cerr -> log_stream ▼ ---
inline GraphData make_base_graph(
    const CoreGraph& core_graph,
    const std::vector<ConnectionRule>& rules,
    int n,
    std::ostream& log_stream // <-- 【追加】
){
    GraphData data; 
    std::map<GridPoint3D, int> coord_to_core_id; 
    std::vector<Point3D> frontier_coords; 
    int next_core_id = 0; 

    Point3D origin_double = {0, 0, 0}; 
    GridPoint3D origin_grid = quantize(origin_double); 
    int origin_core_id = next_core_id++; 

    coord_to_core_id[origin_grid] = origin_core_id; 
    frontier_coords.push_back(origin_double); 
    data.core_locations[origin_core_id] = origin_double; 

    log_stream << "Placed core " << origin_core_id << " at (0, 0, 0)" << std::endl;
    for (int i = 0; i < core_graph.edgeSize(); ++i) {
        const auto& edge = core_graph.edgeInfo(i);
        std::string u_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v1);
        std::string v_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v2);
        data.full_graph.addEdge(u_name, v_name);
        log_stream << "  Connecting " << u_name << " to " << v_name << std::endl;
    }

    for (int i = 0; i < n; ++i) {
        std::vector<Point3D> next_frontier_coords; 
        for (const auto& current_coord_double : frontier_coords) {
            GridPoint3D current_coord_grid = quantize(current_coord_double); 
            int current_core_id = coord_to_core_id.at(current_coord_grid);

            for (const auto& rule : rules) {
                Point3D next_coord_double = current_coord_double + rule.vector; 
                GridPoint3D next_coord_grid = quantize(next_coord_double);   
                int destination_core_id; 

                if (coord_to_core_id.count(next_coord_grid)) {
                    destination_core_id = coord_to_core_id.at(next_coord_grid);
                } else {
                    destination_core_id = next_core_id++; 
                    coord_to_core_id[next_coord_grid] = destination_core_id; 
                    next_frontier_coords.push_back(next_coord_double); 
                    data.core_locations[destination_core_id] = next_coord_double; 

                    log_stream << "Placed core " << destination_core_id << " at ("
                              << next_coord_double.x << ", " << next_coord_double.y << ", " << next_coord_double.z << ")" << std::endl;
                    for (int j = 0; j < core_graph.edgeSize(); ++j) {
                        const auto& edge = core_graph.edgeInfo(j);
                        std::string u_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v1);
                        std::string v_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v2);
                        data.full_graph.addEdge(u_name, v_name);
                        log_stream << "  Connecting " << u_name << " to " << v_name << std::endl;
                    }
                }

                int id1 = std::min(current_core_id, destination_core_id);
                int id2 = std::max(current_core_id, destination_core_id);
                if (id1 == id2 || data.core_connectivity.count({id1, id2})) {
                    continue;
                }
                data.core_connectivity.insert({id1, id2});

                for (const auto& conn : rule.connections) {
                    std::string u_name = std::to_string(current_core_id) + "_" + conn.first;
                    std::string v_name = std::to_string(destination_core_id) + "_" + conn.second;
                    data.full_graph.addEdge(u_name, v_name);
                    log_stream << "  Connecting " << u_name << " to " << v_name << std::endl;
                }
            } 
        } 

        frontier_coords = next_frontier_coords;
        if (frontier_coords.empty()) break;
    } 

    data.full_graph.update();
    return data;
}
// --- ▲ 【修正】 ▲ ---

#endif // MAKE_BASE_GRAPH_HPP