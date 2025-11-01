#ifndef OBJ_TYPES_HPP
#define OBJ_TYPES_HPP

#include <vector>
#include "1_core_graph/MakeBaseGraph.hpp" // Point3D 構造体を再利用するため

/**
 * @brief .txtファイルから読み込んだ頂点タイプごとの
 * メッシュデータを格納する構造体
 */
struct ObjMesh {
    // 頂点座標 ("v ...") のリスト
    std::vector<Point3D> vertices;

    // 面 ("f ...") のリスト
    // "f 1 2 3 4" の場合、{0, 1, 2, 3} (0-based) として格納
    std::vector<std::vector<int>> faces; 
};

#endif // OBJ_TYPES_HPP