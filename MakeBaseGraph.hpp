#ifndef MAKE_BASE_GRAPH_HPP
#define MAKE_BASE_GRAPH_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <iostream>
#include <cmath> // std::fabs を使うために追加
#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/Graph.hpp>

// 座標をdouble型で扱う
struct Point3D {
    double x, y, z;
    // このoperator<は変更しません。
    // std::mapが安全に動作するために、厳密な比較を行う必要があります。
    bool operator<(const Point3D& other) const { 
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z); 
    }
    Point3D operator+(const Point3D& other) const { 
        return {x + other.x, y + other.y, z + other.z}; 
    }
};

//許容誤差(0.1)を考慮した比較関数（Comparator）
struct Point3DComparator {
    // 許容誤差（これ未満なら「同じ」とみなす）
    const double TOLERANCE = 0.1;

    /**
     * @brief 2つの座標が許容誤差の範囲内で「ほぼ同じ」か判定します。
     */
    bool areClose(const Point3D& a, const Point3D& b) const {
        return std::fabs(a.x - b.x) < TOLERANCE &&
               std::fabs(a.y - b.y) < TOLERANCE &&
               std::fabs(a.z - b.z) < TOLERANCE;
    }

    /**
     * @brief std::map がキーを並べ替えるために使用する比較関数。
     * 「a が b より小さいか」を返します。
     */
    bool operator()(const Point3D& a, const Point3D& b) const {
        // 1. まず、許容誤差の範囲で「等価」かどうかをチェック
        if (areClose(a, b)) {
            return false; // 等価なものは「より小さく」ない
        }
        
        // 2. 等価でない場合は、通常の辞書式順序で厳密に比較
        return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
    }
};


// (CoreGraph, ConnectionRule, GraphData 構造体は変更なし)
using CoreGraph = tdzdd::Graph;

struct ConnectionRule {
    Point3D vector;
    std::vector<std::pair<std::string, std::string>> connections;
};

struct GraphData {
    tdzdd::Graph full_graph;
    std::map<int, Point3D> core_locations;
    std::set<std::pair<int, int>> core_connectivity;
};


inline GraphData make_base_graph(
    const CoreGraph& core_graph,
    const std::vector<ConnectionRule>& rules,
    int n)
{
    GraphData data;
    
    // ▼▼▼【型変更】std::mapの定義に、新しい比較関数(Point3DComparator)を指定 ▼▼▼
    std::map<Point3D, int, Point3DComparator> coord_to_core_id;
    
    std::vector<Point3D> frontier;
    int next_core_id = 0;

    Point3D origin = {0, 0, 0};
    int origin_core_id = next_core_id++;
    
    coord_to_core_id[origin] = origin_core_id; // mapがComparatorを使ってキーを保存
    frontier.push_back(origin);
    data.core_locations[origin_core_id] = origin; 

    // (ログ出力とコア内部接続)
    std::cerr << "Placed core " << origin_core_id << " at (0, 0, 0)" << std::endl;
    for (int i = 0; i < core_graph.edgeSize(); ++i) {
        const auto& edge = core_graph.edgeInfo(i);
        std::string u_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v1);
        std::string v_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v2);
        data.full_graph.addEdge(u_name, v_name);
        std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
    }


    for (int i = 0; i < n; ++i) {
        std::vector<Point3D> next_frontier;
        for (const auto& current_coord : frontier) {
            int current_core_id = coord_to_core_id.at(current_coord);

            for (const auto& rule : rules) {
                Point3D next_coord = current_coord + rule.vector;
                int destination_core_id;

                // ▼▼▼【動作変更】▼▼▼
                // 以下の `count` と `at` は、Point3DComparator を使って
                // 許容誤差 0.1 の範囲で「ほぼ同じ」座標を自動的に探します。
                if (coord_to_core_id.count(next_coord)) {
                    destination_core_id = coord_to_core_id.at(next_coord);
                } else {
                    destination_core_id = next_core_id++;
                    coord_to_core_id[next_coord] = destination_core_id;
                    next_frontier.push_back(next_coord);
                    data.core_locations[destination_core_id] = next_coord; 

                    // (コア配置のログと内部接続の追加)
                    std::cerr << "Placed core " << destination_core_id << " at (" << next_coord.x << ", " << next_coord.y << ", " << next_coord.z << ")" << std::endl;
                    for (int j = 0; j < core_graph.edgeSize(); ++j) {
                        const auto& edge = core_graph.edgeInfo(j);
                        std::string u_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v1);
                        std::string v_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v2);
                        data.full_graph.addEdge(u_name, v_name);
                        std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
                    }
                }
                
                // (重複接続のチェックと詳細ログの出力)
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