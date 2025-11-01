# -*- coding: utf-8 -*-

import rhinoscriptsyntax as rs
import Rhino
import System.IO

def export_selected_to_obj_text():
    """
    Rhinoで選択されたポリサーフェスを「Explode（分解）」し、
    個々のサーフェス（BrepFace）の頂点トポロジーをたどることで、
    v（頂点）とf（N角形の面）のテキストを正しく出力します。
    """
    
    # 1. オブジェクト選択 (ポリサーフェスのみ)
    obj_id = rs.GetObject("Select a single Polysurface to export", 16, True, False)
    if not obj_id:
        print("Export cancelled: No object selected.")
        return

    # 画面描画を一時停止
    rs.EnableRedraw(False)

    try:
        # --- ▼ 新アルゴリズム: Explode ▼ ---
        
        # 2. ポリサーフェスを個々のサーフェスに分解
        # (注: 元のオブジェクトは非表示になるか、削除されます)
        surface_ids = rs.ExplodePolysurfaces(obj_id)
        if not surface_ids:
            raise Exception("Failed to explode Polysurface.")

        print("Info: Polysurface exploded into {0} surfaces.".format(len(surface_ids)))

        master_vertices = [] # 座標 (v 行) を格納するリスト
        vertex_map = {}      # 座標(文字列キー) -> インデックス(int) の辞書
        face_list = []       # 面定義 (f 行) を格納するリスト
        
        # 3. 分解された個々のサーフェスをループ処理
        for s_id in surface_ids:
            # サーフェスをBrepFaceオブジェクトとして取得
            brep_face = rs.coercebrep(s_id).Faces[0]
            loop = brep_face.OuterLoop # 面の外周ループを取得
            if not loop: continue
            
            trims = loop.Trims # ループを構成するトリム（辺）を取得
            current_face_indices = [] # この面を構成する頂点のインデックス
            
            # 4. 面の各「頂点」をたどる
            for trim in trims:
                vertex = trim.StartVertex
                if not vertex: continue
                
                pt = vertex.Location # 頂点の Point3d 座標

                # 座標を小数第3位で丸め、辞書のキーにする
                x = round(pt.X, 3)
                y = round(pt.Y, 3)
                z = round(pt.Z, 3)
                # -0.0 を 0.0 に補正
                if x == -0.0: x = 0.0
                if y == -0.0: y = 0.0
                if z == -0.0: z = 0.0

                pt_key = "v {0:.3f} {1:.3f} {2:.3f}".format(x, y, z)

                master_index = -1
                
                # 5. 頂点マップを使い、重複しないvリストを作成
                if pt_key in vertex_map:
                    # この頂点は既に追加済み
                    master_index = vertex_map[pt_key]
                else:
                    # 新しい頂点
                    master_index = len(master_vertices)
                    vertex_map[pt_key] = master_index
                    master_vertices.append(pt_key) # "v 1.23 4.56 7.89" の文字列
                
                if master_index not in current_face_indices:
                    current_face_indices.append(master_index)
            
            # 6. 面（N角形）の定義を追加
            face_list.append(current_face_indices)
            
        # --- ▲ 新アルゴリズム終了 ▲ ---

        print("Processing complete: {0} vertices, {1} faces.".format(len(master_vertices), len(face_list)))

        # 7. テキスト出力
        output_lines = []
        
        # v (頂点) リストの作成
        output_lines.append("# --- Vertices ({0}) ---".format(len(master_vertices)))
        output_lines.extend(master_vertices) # 既に "v ..." 形式

        # f (面) リストの作成
        output_lines.append("\n# --- Faces ({0}) ---".format(len(face_list)))
        ngon_count = 0
        for indices in face_list:
            if len(indices) < 3: continue
            
            # OBJは1-based index
            obj_indices = [str(idx + 1) for idx in indices]
            output_lines.append("f " + " ".join(obj_indices))
            ngon_count += 1
            
        final_text = "\n".join(output_lines)
        
        # 8. クリップボードとコマンドラインに出力
        try:
            rs.ClipboardText(final_text)
            print("Success: 'v' and 'f' data ({0} Ngons) copied.".format(ngon_count))
        except Exception as e:
            print("Warning: Could not copy to clipboard. {0}".format(e))

        print("--- Start of OBJ data ---")
        print(final_text)
        print("--- End of OBJ data ---")

    except Exception as e:
        print("Error: {0}".format(e))
    finally:
        # 9. 分解したサーフェスを削除し、画面を再描画
        if 'surface_ids' in locals() and surface_ids:
            rs.DeleteObjects(surface_ids)
        
        # 元のオブジェクトを復元 (Undo)
        # ユーザーが元のオブジェクトを残したい場合、Undoを実行
        print("Info: Exploded surfaces deleted. Run 'Undo' if you want to restore the original polysurface.")
        
        rs.EnableRedraw(True)


if __name__ == "__main__":
    export_selected_to_obj_text()