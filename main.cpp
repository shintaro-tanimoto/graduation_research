#include "MakeBaseGraph.hpp"
#include <string>
#include <fstream>
#include <iostream>

/**
 * @brief 生成されたグラフデータをRhino 3Dのスクリプトで読み込める形式のテキストファイルに出力します。
 * @param data グラフデータ(座標リストと辺情報)。
 * @param filename 出力ファイル名。
 */

void exportGraphForRhino(const GraphData& data, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    // 1. 全ての頂点座標を出力
    for (const auto& coord : data.coordinates) {
        ofs << coord.x << " " << coord.y << " " << coord.z << std::endl;
    }

    // 2. 座標と辺のセクションを区切るためのセパレータ
    ofs << "---EDGES---" << std::endl;

    // 3. 全ての辺の接続情報を、頂点のインデックスで出力
    for (int i = 0; i < data.graph.edgeSize(); ++i) {
        const tdzdd::Graph::EdgeInfo& edge = data.graph.edgeInfo(i);

        // edge.v1, v2はtdzdd内部IDであり、座標リストのインデックスとは異なる。
        // そのため、まず内部IDから頂点名("0001"など)を取得する。
        std::string v1_name = data.graph.vertexName(edge.v1);
        std::string v2_name = data.graph.vertexName(edge.v2);

        // 次に、頂点名を整数に変換して正しいインデックスを得る。
        try {
            int index1 = std::stoi(v1_name);
            int index2 = std::stoi(v2_name);
            ofs << index1 << " " << index2 << std::endl;
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error converting vertex name to integer: " << e.what() << std::endl;
        }
    }

    std::cout << "Graph data for Rhino was written to " << filename << std::endl;
}

/**
 * @brief メイン関数
 */
int main() {
    // === 入力設定 ===
    // グラフの基本構造を定義するベクトル集合 (例: 3次元の6方向)
    std::vector<Point3D> vectors = {
        {1, 0, 0}, {-1, 0, 0},
        {0, 1, 0}, {0, -1, 0},
        {0, 0, 1}, {0, 0, -1}
    };
    int n = 3; // グラフを成長させるステップ数

    // === グラフ生成 ===
    std::cout << "Generating a graph with n = " << n << "..." << std::endl;
    GraphData data = make_base_graph(vectors, n);

    // === Rhino用ファイル出力 ===
    exportGraphForRhino(data, "graph_data.txt");
    
    return 0;
}