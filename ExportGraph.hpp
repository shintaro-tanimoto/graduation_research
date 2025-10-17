#ifndef EXPORT_GRAPH_HPP
#define EXPORT_GRAPH_HPP

#include "MakeBaseGraph.hpp" // GraphData構造体などを利用するためにインクルード

/**
 * @brief 【コア骨格用】コアの配置点とコア間接続をRhino用ファイルに出力します。
 * @param data グラフデータ(コアの場所と接続情報を含む)。
 * @param filename 出力ファイル名。
 */
inline void exportCoreConnectivityForRhino(const GraphData& data, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    std::map<int, int> core_id_to_index;
    int current_index = 0;

    // 1. 全てのコアの座標を出力しつつ、インデックスを割り当てる
    for (const auto& pair : data.core_locations) {
        int core_id = pair.first;
        const Point3D& coord = pair.second;
        
        ofs << coord.x << " " << coord.y << " " << coord.z << std::endl;
        core_id_to_index[core_id] = current_index++;
    }

    // 2. 頂点と辺のセクションを区切るためのセパレータ
    ofs << "---EDGES---" << std::endl;

    // 3. 全てのコア間接続を、割り当てたインデックスで出力
    for (const auto& connection : data.core_connectivity) {
        int id1 = connection.first;
        int id2 = connection.second;
        
        ofs << core_id_to_index.at(id1) << " " << core_id_to_index.at(id2) << std::endl;
    }

    std::cerr << "Core connectivity data for Rhino was written to " << filename << std::endl;
}

/**
 * @brief 【内容確認用】全体の詳細なグラフを.dot形式でファイルに出力します。
 * @param graph 出力するグラフオブジェクト。
 * @param filename 出力先のファイル名。
 */
inline void exportFullGraphForChecking(const tdzdd::Graph& graph, const std::string& filename) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    ofs << "graph G {" << std::endl;
    ofs << "  node [shape=circle];" << std::endl;
    for (int i = 0; i < graph.edgeSize(); ++i) {
        const auto& edge = graph.edgeInfo(i);
        ofs << "  \"" << graph.vertexName(edge.v1) << "\" -- \"" << graph.vertexName(edge.v2) << "\";" << std::endl;
    }
    ofs << "}" << std::endl;
    std::cerr << "Full graph data for checking was written to " << filename << std::endl;
}

#endif // EXPORT_GRAPH_HPP