#ifndef DUAL_GRAPH_HPP
#define DUAL_GRAPH_HPP

#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm> // std::min, std::max

#include "3_geometry/ObjTypes.hpp"
#include "tdzdd/util/Graph.hpp" 
#include "1_core_graph/MakeBaseGraph.hpp" // <-- 【追加】 GridPoint3D, quantize() のため

/**
 * @brief ObjMesh から双対グラフ (Dual Graph) を構築します。
 * (修正: 頂点インデックスではなく、量子化された座標で辺を判定)
 */
inline tdzdd::Graph buildDualGraph(const ObjMesh& mesh) {
    
    tdzdd::Graph dual_graph;
    
    // --- ▼ 修正点: キーを <int, int> から <GridPoint3D, GridPoint3D> に変更 ▼ ---
    // 量子化された辺 (v1, v2) -> この辺を共有する面のインデックス (のリスト)
    std::map<std::pair<GridPoint3D, GridPoint3D>, std::vector<int>> edge_to_faces_map;
    // --- ▲ 修正点 ▲ ---

    // 1. 辺 -> 面 のマップを構築
    for (int i = 0; i < mesh.faces.size(); ++i) {
        const auto& face = mesh.faces[i]; 
        
        for (size_t j = 0; j < face.size(); ++j) {
            int v1_idx = face[j];
            int v2_idx = face[(j + 1) % face.size()]; 

            // --- ▼ 修正点: 頂点インデックス -> 量子化座標 に変更 ▼ ---
            
            // 頂点座標を取得
            const Point3D& p1 = mesh.vertices.at(v1_idx);
            const Point3D& p2 = mesh.vertices.at(v2_idx);

            // 座標を量子化 (TOLERANCE = 0.1)
            GridPoint3D g1 = quantize(p1);
            GridPoint3D g2 = quantize(p2);

            // 辺を正規化 (GridPoint3D は MakeBaseGraph.hpp で operator< が定義済み)
            std::pair<GridPoint3D, GridPoint3D> edge_key = {std::min(g1, g2), std::max(g1, g2)};
            
            // --- ▲ 修正点 ▲ ---

            // この辺(edge_key)は、面(i)に属している
            edge_to_faces_map[edge_key].push_back(i);
        }
    }

    // 2. 双対グラフのエッジを構築
    for (const auto& pair : edge_to_faces_map) {
        const auto& faces_sharing_this_edge = pair.second;
        
        // 1つの辺(量子化済み)を2つの面が共有している場合
        if (faces_sharing_this_edge.size() == 2) {
            int face_idx_1 = faces_sharing_this_edge[0];
            int face_idx_2 = faces_sharing_this_edge[1];
            
            dual_graph.addEdge(std::to_string(face_idx_1), std::to_string(face_idx_2));
        }
    }
    
    dual_graph.update();
    return dual_graph;
}

#endif // DUAL_GRAPH_HPP