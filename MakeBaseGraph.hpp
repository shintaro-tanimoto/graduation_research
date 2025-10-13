#ifndef MAKE_BASE_GRAPH_HPP
#define MAKE_BASE_GRAPH_HPP

#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <tdzdd/util/MessageHandler.hpp>
#include <tdzdd/util/Graph.hpp>

/**
 * @struct Point3D
 * @brief 3次元の座標またはベクトルを表現する構造体。
 * @note std::mapのキーとして使用するため、比較演算子(<)をオーバーロードしています。
 */
struct Point3D {
    int x, y, z; // 座標成分

    // 座標を辞書式順序で比較するための比較演算子
    bool operator<(const Point3D& other) const {
        return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
    }
};

/**
 * @struct GraphData
 * @brief 生成されたグラフとその頂点座標をまとめて保持する構造体。
 */
struct GraphData {
    tdzdd::Graph graph;           // tdzdd形式のグラフデータ
    std::vector<Point3D> coordinates; // グラフの頂点IDに対応する3D座標のリスト
};

/**
 * @brief 3次元格子状のグラフを指定されたステップ数で生成します。
 * @param vectors グラフの基本構造を定義するベクトル（1頂点から伸びる辺）の集合。
 * @param n グラフを成長させるステップ数（反復回数）。
 * @return 生成されたグラフデータ(GraphData)。
 */
inline GraphData make_base_graph(const std::vector<Point3D>& vectors, int n) {
    // グラフ関連の変数を初期化
    tdzdd::Graph graph;
    std::map<Point3D, std::string> coord_to_vertex_name; // 座標から頂点名へのマップ
    std::vector<Point3D> vertex_coordinates;             // 頂点ID(インデックス)から座標へのリスト
    std::vector<Point3D> frontier;                       // 各ステップで処理対象となる最前線の頂点群

    // 初期設定
    Point3D origin = {0, 0, 0}; // 原点座標
    int next_vertex_idx = 0;    // 次に割り当てる頂点ID (インデックス)

    // Step 1: 原点(0,0,0)に最初の頂点を生成
    std::string origin_name = std::to_string(next_vertex_idx++);
    coord_to_vertex_name[origin] = origin_name;
    vertex_coordinates.push_back(origin);
    frontier.push_back(origin);

    // debug
    std::cout << "Added vertex " << origin_name << " at (0, 0, 0)" << std::endl;
    std::cout << "<<<<" << std::endl;

    for (int i = 0; i < n; ++i) {
        std::vector<Point3D> next_frontier; // 次のステップのフロンティア

        // 現在のフロンティアにある各頂点からグラフを拡張
        for (const auto& current_coord : frontier) {
            std::string u_name = coord_to_vertex_name.at(current_coord);

            // 定義された各ベクトル方向に辺を伸ばす
            for (const auto& vec : vectors) {
                // 新しい座標を計算
                Point3D next_coord = {
                    current_coord.x + vec.x,
                    current_coord.y + vec.y,
                    current_coord.z + vec.z
                };
                std::string v_name;

                // 新しい座標にまだ頂点がなければ、新しい頂点を作成
                if (coord_to_vertex_name.find(next_coord) == coord_to_vertex_name.end()) {
                    v_name = std::to_string(next_vertex_idx++);
                    coord_to_vertex_name[next_coord] = v_name;
                    vertex_coordinates.push_back(next_coord);
                    next_frontier.push_back(next_coord); // 新規頂点を次のフロンティアに追加

                    // デバッグ出力
                    std::cout << "Added vertex " << v_name << " at (" << next_coord.x << ", " << next_coord.y << ", " << next_coord.z << ")" << std::endl;
                    std::cout << "<<<<" << std::endl;
                } else {
                    // 座標が既存の場合は、その頂点の名前を取得
                    v_name = coord_to_vertex_name.at(next_coord);
                }
                
                // 現在の頂点と新しい(または既存の)頂点の間に辺を追加
                graph.addEdge(u_name, v_name);

                // デバッグ出力
                std::cout << "Added edge between " << u_name << " and " << v_name << std::endl;
                std::cout << "<<<<" << std::endl;
            }
        }
        frontier = next_frontier; // フロンティアを更新
        if (frontier.empty()) break; // 拡張する頂点がなくなったら終了
    }

    graph.update(); // tdzddグラフの内部構造を最終化
    return {graph, vertex_coordinates}; // 生成したグラフデータを返す
}


/**
 * @brief tdzdd::GraphオブジェクトをDOT形式でファイルに出力します。
 * @param graph 出力するグラフオブジェクト。
 * @param filename 出力先のファイル名。
 * @note DOT形式はGraphvizなどのツールで可視化できます。
 */
inline void exportGraphToDot(const tdzdd::Graph& graph, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    ofs << "graph G {" << std::endl;
    ofs << "  node [shape=circle];" << std::endl;

    // グラフの全ての辺をループで処理
    for (int i = 0; i < graph.edgeSize(); ++i) {
        const tdzdd::Graph::EdgeInfo& edge = graph.edgeInfo(i);
        // 辺の両端の頂点名を取得して書き出す
        ofs << "  \"" << graph.vertexName(edge.v1) << "\" -- \"" << graph.vertexName(edge.v2) << "\";" << std::endl;
    }

    ofs << "}" << std::endl;
    std::cout << "Graph data was written to " << filename << std::endl;
}

#endif // MAKE_BASE_GRAPH_HPP