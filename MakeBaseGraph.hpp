#ifndef MAKE_BASE_GRAPH_HPP
#define MAKE_BASE_GRAPH_HPP

#include <vector>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <iostream>
#include <fstream> // Included for potential future use, though not directly used by make_base_graph
#include <cmath> // std::round, std::fabs を使うために追加
#include <tdzdd/util/MessageHandler.hpp> // Included for potential future use
#include <tdzdd/util/Graph.hpp>

// 座標をdouble型で扱う
struct Point3D {
    double x, y, z;
    Point3D operator+(const Point3D& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
};

// 【新設】std::mapのキーとして使う、整数化されたグリッド座標
struct GridPoint3D {
    long long x_grid, y_grid, z_grid; // 整数型にする
    // map のキーとして使えるように比較演算子を定義
    bool operator<(const GridPoint3D& other) const {
        return std::tie(x_grid, y_grid, z_grid) < std::tie(other.x_grid, other.y_grid, other.z_grid);
    }
};

// 【新設】double座標をグリッド座標に変換するヘルパー関数
const double TOLERANCE = 0.1; // 許容誤差
const double QUANTIZATION_FACTOR = 1.0 / TOLERANCE; // 量子化係数 (0.1なら10)
inline GridPoint3D quantize(const Point3D& p) {
    // 各座標値を係数倍して四捨五入し、整数に変換
    return {
        static_cast<long long>(std::round(p.x * QUANTIZATION_FACTOR)),
        static_cast<long long>(std::round(p.y * QUANTIZATION_FACTOR)),
        static_cast<long long>(std::round(p.z * QUANTIZATION_FACTOR))
    };
}

// コアグラフの型定義 (変更なし)
using CoreGraph = tdzdd::Graph;

// 接続ルールの構造体 (変更なし)
struct ConnectionRule {
    Point3D vector;
    std::vector<std::pair<std::string, std::string>> connections;
};

// 返り値のデータ構造 (変更なし)
struct GraphData {
    tdzdd::Graph full_graph;
    std::map<int, Point3D> core_locations; // コアの座標はdoubleで保持
    std::set<std::pair<int, int>> core_connectivity;
};


inline GraphData make_base_graph(
    const CoreGraph& core_graph,
    const std::vector<ConnectionRule>& rules,
    int n)
{
    GraphData data; // 結果を格納するオブジェクト

    // ▼▼▼ mapのキーを GridPoint3D に変更 ▼▼▼
    std::map<GridPoint3D, int> coord_to_core_id;

    std::vector<Point3D> frontier_coords; // フロンティアの座標はdoubleで保持
    int next_core_id = 0; // 次に割り当てるコアID

    // --- Step 1: 原点に最初のコアを配置 ---
    Point3D origin_double = {0, 0, 0}; // double型の原点座標
    GridPoint3D origin_grid = quantize(origin_double); // map検索用に量子化
    int origin_core_id = next_core_id++; // 最初のコアID (0)

    coord_to_core_id[origin_grid] = origin_core_id; // 量子化座標をキーとして登録
    frontier_coords.push_back(origin_double); // フロンティアにはdouble座標を追加
    data.core_locations[origin_core_id] = origin_double; // コア位置もdoubleで記録

    // 最初のコアの内部接続をログ出力＆グラフに追加
    std::cerr << "Placed core " << origin_core_id << " at (0, 0, 0)" << std::endl;
    for (int i = 0; i < core_graph.edgeSize(); ++i) {
        const auto& edge = core_graph.edgeInfo(i);
        std::string u_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v1);
        std::string v_name = std::to_string(origin_core_id) + "_" + core_graph.vertexName(edge.v2);
        data.full_graph.addEdge(u_name, v_name);
        std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
    }

    // --- Step 2-4: n回ステップを繰り返しグラフを成長 ---
    for (int i = 0; i < n; ++i) {
        std::vector<Point3D> next_frontier_coords; // 次のフロンティア（double座標）

        // 現在のフロンティアの各座標についてループ
        for (const auto& current_coord_double : frontier_coords) {
            GridPoint3D current_coord_grid = quantize(current_coord_double); // map検索用に量子化
            // 量子化座標を使ってコアIDを取得 (存在しないはずはないのでatを使用)
            int current_core_id = coord_to_core_id.at(current_coord_grid);

            // 定義された各接続ルールについてループ
            for (const auto& rule : rules) {
                // 次のコアの座標をdoubleで計算
                Point3D next_coord_double = current_coord_double + rule.vector;
                // map検索/登録用に量子化
                GridPoint3D next_coord_grid = quantize(next_coord_double);
                int destination_core_id; // 接続先のコアID

                // ▼▼▼ 量子化されたキーで map を検索 ▼▼▼
                if (coord_to_core_id.count(next_coord_grid)) {
                    // 既に（ほぼ）同じ座標にコアが存在する場合
                    destination_core_id = coord_to_core_id.at(next_coord_grid);
                } else {
                    // 新しい座標にコアを配置する場合
                    destination_core_id = next_core_id++; // 新しいコアIDを取得
                    coord_to_core_id[next_coord_grid] = destination_core_id; // 量子化キーで登録
                    next_frontier_coords.push_back(next_coord_double); // 次フロンティアにはdouble座標
                    data.core_locations[destination_core_id] = next_coord_double; // doubleで位置記録

                    // 新しいコアの配置ログと内部接続
                    std::cerr << "Placed core " << destination_core_id << " at ("
                              << next_coord_double.x << ", " << next_coord_double.y << ", " << next_coord_double.z << ")" << std::endl;
                    for (int j = 0; j < core_graph.edgeSize(); ++j) {
                        const auto& edge = core_graph.edgeInfo(j);
                        std::string u_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v1);
                        std::string v_name = std::to_string(destination_core_id) + "_" + core_graph.vertexName(edge.v2);
                        data.full_graph.addEdge(u_name, v_name);
                        std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
                    }
                }

                // 重複接続チェックと辺の追加
                int id1 = std::min(current_core_id, destination_core_id);
                int id2 = std::max(current_core_id, destination_core_id);
                // 自己ループまたは既に接続済みならスキップ
                if (id1 == id2 || data.core_connectivity.count({id1, id2})) {
                    continue;
                }
                // 新しい接続ペアを記録
                data.core_connectivity.insert({id1, id2});

                // ルールに基づいて詳細グラフの辺を追加＆ログ出力
                for (const auto& conn : rule.connections) {
                    std::string u_name = std::to_string(current_core_id) + "_" + conn.first;
                    std::string v_name = std::to_string(destination_core_id) + "_" + conn.second;
                    data.full_graph.addEdge(u_name, v_name);
                    std::cerr << "  Connecting " << u_name << " to " << v_name << std::endl;
                }
            } // end loop rules
        } // end loop frontier_coords

        // フロンティアを更新
        frontier_coords = next_frontier_coords;
        // 次のフロンティアが空ならループ終了
        if (frontier_coords.empty()) break;
    } // end loop n

    // グラフ構造を最終化
    data.full_graph.update();
    // 結果を返す
    return data;
}

#endif // MAKE_BASE_GRAPH_HPP