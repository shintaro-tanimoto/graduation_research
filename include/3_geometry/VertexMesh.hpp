#ifndef VERTEX_MESH_HPP
#define VERTEX_MESH_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <iostream> // デバッグ用

#include "3_geometry/ObjTypes.hpp"
#include "1_core_graph/MakeBaseGraph.hpp" // GraphData

// ヘルパー: 頂点名 (例: "1_b") から コアID (1) と タイプ ("b") を抽出
// (ConstrainedSearch.hpp にある getBaseType と似ているが、IDも取得する)
inline bool parseVertexName(const std::string& full_name, int& core_id, std::string& base_type) {
    size_t underscore = full_name.find('_');
    if (underscore == std::string::npos) {
        return false; // 不正な形式
    }
    try {
        // "1" の部分を整数に
        core_id = std::stoi(full_name.substr(0, underscore));
        // "b" の部分
        base_type = full_name.substr(underscore + 1);
        return true;
    } catch (const std::exception& e) {
        // std::stoi 失敗など
        return false;
    }
}

/**
 * @brief 指定された頂点名に対応するメッシュを、正しい座標に平行移動して生成します。
 * * @param vertex_name "0_a" や "1_b" などの頂点名
 * @param base_data グラフ全体のデータ (コアの座標情報を含む)
 * @param mesh_data 頂点タイプ別のメッシュ定義 (GraphLoaderが読み込んだもの)
 * @return 平行移動済みの ObjMesh インスタンス
 */
inline ObjMesh getMeshForVertex(
    const std::string& vertex_name,
    const GraphData& base_data,
    const std::map<std::string, ObjMesh>& mesh_data
) {
    int core_id;
    std::string base_type;

    // 1. 頂点名をパース
    if (!parseVertexName(vertex_name, core_id, base_type)) {
        throw std::runtime_error("Error: Invalid vertex name format: " + vertex_name);
    }

    // 2. 該当するメッシュ定義 (テンプレート) を探す
    if (mesh_data.count(base_type) == 0) {
        throw std::runtime_error("Error: No mesh data found for type: " + base_type);
    }
    const ObjMesh& template_mesh = mesh_data.at(base_type);

    // 3. 該当するコアの座標 (平行移動量) を探す
    if (base_data.core_locations.count(core_id) == 0) {
        throw std::runtime_error("Error: No location data found for core ID: " + std::to_string(core_id));
    }
    const Point3D& translation = base_data.core_locations.at(core_id);

    // 4. 新しいメッシュ (コピー) を作成
    ObjMesh translated_mesh = template_mesh; // (面情報はそのままコピーされる)

    // 5. すべての頂点を平行移動
    // (template_mesh.vertices を上書きするのではなく、
    //  translated_mesh.vertices を変更する)
    for (Point3D& v : translated_mesh.vertices) {
        v = v + translation; // (Point3D の + 演算子を使用)
    }

    return translated_mesh;
}

#endif // VERTEX_MESH_HPP