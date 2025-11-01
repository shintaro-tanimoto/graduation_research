#ifndef EXPORT_GRAPH_HPP
#define EXPORT_GRAPH_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <iomanip> 
#include <ostream> // <-- 【追加】

#include "1_core_graph/MakeBaseGraph.hpp"
#include "3_geometry/ObjTypes.hpp" 

/**
 * @brief 頂点名 "ID_TYPE" を "TYPE_ID" 形式に変換します。
 */
inline std::string remapVertexName(const std::string& original_name) {
    size_t separator_pos = original_name.find('_');
    if (separator_pos == std::string::npos) {
        return original_name;
    }
    std::string core_id = original_name.substr(0, separator_pos);
    std::string type = original_name.substr(separator_pos + 1);
    return type + "_" + core_id; // TYPE_ID 形式で結合
}

// --- ▼ 【修正】 全ての関数に std::ostream& log_stream を追加 ▼ ---

/**
 * @brief 【コア骨格用】コアの配置点とコア間接続をRhino用ファイルに出力します。
 */
inline void exportCoreConnectivityForRhino(const GraphData& data, const std::string& filename, std::ostream& log_stream) {
    std::ofstream ofs(filename);
    if (!ofs) {
        log_stream << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    std::map<int, int> core_id_to_index;
    int current_index = 0;
    for (const auto& pair : data.core_locations) {
        int core_id = pair.first;
        const Point3D& coord = pair.second;
        ofs << coord.x << " " << coord.y << " " << coord.z << std::endl; 
        core_id_to_index[core_id] = current_index++;
    }
    ofs << "---EDGES---" << std::endl;
    for (const auto& connection : data.core_connectivity) {
        int id1 = connection.first;
        int id2 = connection.second;
        ofs << core_id_to_index.at(id1) << " " << core_id_to_index.at(id2) << std::endl;
    }
    log_stream << "Core connectivity data for Rhino was written to " << filename << std::endl;
}

/**
 * @brief 【内容確認用】全体の詳細なグラフを.dot形式でファイルに出力します。
 */
inline void exportFullGraphForChecking(const tdzdd::Graph& graph, const std::string& filename, std::ostream& log_stream) {
    std::ofstream ofs(filename);
    if (!ofs) {
        log_stream << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    ofs << "graph G {" << std::endl;
    ofs << "  node [shape=circle];" << std::endl;
    for (int i = 0; i < graph.edgeSize(); ++i) {
        const auto& edge = graph.edgeInfo(i);
        std::string v1_name_original = graph.vertexName(edge.v1);
        std::string v2_name_original = graph.vertexName(edge.v2);
        
        std::string v1_name_remapped = remapVertexName(v1_name_original);
        std::string v2_name_remapped = remapVertexName(v2_name_original);

        ofs << "  \"" << v1_name_remapped << "\" -- \"" << v2_name_remapped << "\";" << std::endl;
    }
    ofs << "}" << std::endl;
    log_stream << "Full graph data for checking was written to " << filename << std::endl;
}

/**
 * @brief ObjMesh 構造体を .obj ファイルとして書き出します (デバッグ用)
 */
inline void exportObjMesh(const ObjMesh& mesh, const std::string& filename, std::ostream& log_stream) {
    std::ofstream ofs(filename);
    if (!ofs) {
        log_stream << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    ofs << "# --- Vertices (" << mesh.vertices.size() << ") ---" << std::endl;
    for (const auto& v : mesh.vertices) {
        ofs << "v " << std::fixed << std::setprecision(3) 
            << v.x << " " << v.y << " " << v.z << std::endl;
    }

    ofs << "\n# --- Faces (" << mesh.faces.size() << ") ---" << std::endl;
    for (const auto& face : mesh.faces) {
        ofs << "f";
        for (int idx : face) {
            ofs << " " << (idx + 1); // 0-based -> 1-based
        }
        ofs << std::endl;
    }
    
    log_stream << "Debug mesh data was written to " << filename << std::endl;
}
// --- ▲ 【修正】 ▲ ---

#endif // EXPORT_GRAPH_HPP