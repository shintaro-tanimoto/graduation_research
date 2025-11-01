#ifndef SOLUTION_MESH_HPP
#define SOLUTION_MESH_HPP

#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream> // std::cerr のため
#include <ostream> // <-- 【追加】 std::ostream のため

#include "3_geometry/ObjTypes.hpp"
#include "1_core_graph/MakeBaseGraph.hpp" 
#include "3_geometry/VertexMesh.hpp"      
#include "9_export/ExportGraph.hpp"      

using FaceKey = std::set<GridPoint3D>;

// --- ▼ 【修正】 関数シグネチャと std::cerr -> log_stream ▼ ---
inline ObjMesh buildSolutionMesh(
    const std::set<std::string>& solution,
    const GraphData& base_data,
    const std::map<std::string, ObjMesh>& mesh_data,
    std::ostream& log_stream // <-- 【追加】
) {
    ObjMesh merged_mesh;
    int vertex_offset = 0;

    // 1. メッシュの統合
    for (const std::string& vertex_name : solution) {
        ObjMesh mesh_part = getMeshForVertex(vertex_name, base_data, mesh_data);
        merged_mesh.vertices.insert(
            merged_mesh.vertices.end(),
            mesh_part.vertices.begin(),
            mesh_part.vertices.end()
        );
        for (const auto& face : mesh_part.faces) {
            std::vector<int> offset_face;
            for (int idx : face) {
                offset_face.push_back(idx + vertex_offset);
            }
            merged_mesh.faces.push_back(offset_face);
        }
        vertex_offset += mesh_part.vertices.size();
    }
    
    log_stream << "  Merged solution mesh: " << merged_mesh.vertices.size() 
              << " vertices, " << merged_mesh.faces.size() << " faces (before cleaning)." << std::endl;

    // 2. 接合面の削除
    std::map<FaceKey, std::vector<int>> face_map;
    for (int i = 0; i < merged_mesh.faces.size(); ++i) {
        const auto& face_indices = merged_mesh.faces[i];
        FaceKey key;
        for (int idx : face_indices) {
            key.insert(quantize(merged_mesh.vertices[idx]));
        }
        face_map[key].push_back(i);
    }

    std::set<int> faces_to_delete;
    for (const auto& pair : face_map) {
        if (pair.second.size() > 1) { 
            faces_to_delete.insert(pair.second.begin(), pair.second.end());
        }
    }

    if (!faces_to_delete.empty()) {
        log_stream << "  Cleaning mesh: Found and marked " 
                  << faces_to_delete.size() << " coincident faces for deletion." << std::endl;
    }

    // 3. クリーニング済みメッシュの作成
    ObjMesh final_mesh;
    final_mesh.vertices = merged_mesh.vertices; 
    for (int i = 0; i < merged_mesh.faces.size(); ++i) {
        if (faces_to_delete.count(i) == 0) {
            final_mesh.faces.push_back(merged_mesh.faces[i]);
        }
    }
    
    log_stream << "  Built final mesh: " << final_mesh.vertices.size() 
              << " vertices, " << final_mesh.faces.size() << " faces." << std::endl;
              
    return final_mesh;
}


/**
 * @brief 1つの解 (頂点セット) に対応するメッシュを .obj ファイルとして出力します。
 */
// (SolutionMesh.hpp の exportSolutionMesh 関数)

inline void exportSolutionMesh(
    const std::set<std::string>& solution,
    const std::string& output_filename,
    const GraphData& base_data,
    const std::map<std::string, ObjMesh>& mesh_data,
    std::ostream& log_stream 
) {
    if (solution.empty()) {
        std::cerr << "Warning: Skipping mesh export for empty solution." << std::endl;
        return;
    }
    
    // 1. 内部関数でメッシュを構築
    ObjMesh final_mesh = buildSolutionMesh(solution, base_data, mesh_data, log_stream); 

    // 2. ファイルに出力
    // --- ▼ 【修正】 log_stream を渡す ▼ ---
    exportObjMesh(final_mesh, output_filename, log_stream);
    // --- ▲ 【修正】 ▲ ---
}
// --- ▲ 【修正】 ▲ ---

#endif // SOLUTION_MESH_HPP