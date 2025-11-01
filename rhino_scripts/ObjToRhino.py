# -*- coding: utf-8 -*-

import rhinoscriptsyntax as rs
import Rhino
import System.IO

def import_obj_to_rhino():
    """
    .obj ファイルを読み込み、その v (頂点) と f (面) 情報に基づいて
    Rhinoのシーン上にメッシュオブジェクトを再構築します。

    C++側が出力する 'v x y z' と 'f 1 2 3 ...' の形式を想定しています。
    """

    # 1. ファイル選択ダイアログを表示
    file_path = rs.OpenFileName("Select .obj file to import", "OBJ files (*.obj)|*.obj|All files (*.*)|*.*")
    if not file_path or not System.IO.File.Exists(file_path):
        print("Import cancelled: No file selected.")
        return

    print("Importing mesh from: {0}".format(file_path))

    vertices = [] # 頂点座標 (Point3d) を格納するリスト
    faces = []    # 面のインデックス (list of lists) を格納するリスト

    # 2. .obj ファイルをパース
    try:
        with System.IO.StreamReader(file_path) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue

                parts = line.split()
                if not parts:
                    continue

                keyword = parts[0]
                args = parts[1:]

                if keyword == 'v':
                    # 頂点 (Vertex)
                    if len(args) >= 3:
                        try:
                            x = float(args[0])
                            y = float(args[1])
                            z = float(args[2])
                            vertices.append(Rhino.Geometry.Point3d(x, y, z))
                        except ValueError:
                            print("Warning: Skipping malformed vertex line: {0}".format(line))

                elif keyword == 'f':
                    # 面 (Face)
                    if len(args) >= 3:
                        face_indices = []
                        valid_face = True
                        for arg in args:
                            try:
                                # OBJは1-based, Rhinoは0-based
                                idx = int(arg) - 1 
                                face_indices.append(idx)
                            except ValueError:
                                # "f 1//2 3//4" のような形式は無視
                                print("Warning: Skipping malformed face line (only simple indices supported): {0}".format(line))
                                valid_face = False
                                break
                        if valid_face:
                            faces.append(face_indices)

        if not vertices:
            raise Exception("No valid 'v' (vertex) lines found in file.")

        # 3. Rhinoメッシュオブジェクトの作成
        mesh = Rhino.Geometry.Mesh()

        # 頂点を追加
        mesh.Vertices.AddVertices(vertices)

        # 面 (N-gon) を追加
        for indices in faces:
            try:
                # この関数が三角、四角、N角形を自動で処理
                mesh.Faces.AddFace(indices) 
            except:
                print("Warning: Failed to add face with indices: {0}".format(indices))

        # 4. メッシュをドキュメントに追加
        doc = Rhino.RhinoDoc.ActiveDoc
        if doc:
            # メッシュが正しく構築されているか確認 (法線計算など)
            mesh.Normals.ComputeNormals()
            mesh.Compact()

            doc.Objects.AddMesh(mesh)
            doc.Views.Redraw()
            print("Success: Mesh imported with {0} vertices and {1} faces.".format(mesh.Vertices.Count, mesh.Faces.Count))
        else:
            raise Exception("No active Rhino document.")

    except Exception as e:
        print("Error: Failed to import OBJ file. {0}".format(e))

if __name__ == "__main__":
    import_obj_to_rhino()