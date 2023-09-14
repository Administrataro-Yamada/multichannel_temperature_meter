import glob, os
from pathlib import Path

#置換リスト
new_replace_extension = {
    "-Edge_Cuts." : "GML",
    "-B_Paste." : "GBP",
    "-B_Cu." : "GBL",
    "-B_Silkscreen." : "GBO",
    "-B_Mask." : "GBS",
    "-F_Paste." : "GTP",
    "-F_Cu." : "GTL",
    "-F_Silkscreen." : "GTO",
    "-F_Mask." : "GTS",
    "-NPTH" : "TXT",
    "-PTH" : "TXT",
}
new_replace_extension_key_list = list(new_replace_extension.keys())
new_replace_extension_value_list = list(new_replace_extension.values())


old_replace_extension_value_list = ["drl", "gbr"]


files = glob.glob(os.getcwd()+"/*")


#変換
gerber_files = []
for file in files:
    for extension in old_replace_extension_value_list:
        if extension in file:
            gerber_files.append(file)

if gerber_files:
    for filename in gerber_files:
        for key in new_replace_extension_key_list:
            if key in filename:
                f = Path(filename)
                print(os.getcwd() + "/" + f.stem + "." + new_replace_extension[key])
                f.rename(os.getcwd() + "/" + f.stem + "." + new_replace_extension[key])

# #逆変換
# else:
#     for file in files:
#         for key in new_replace_extension_key_list:
#             if key in file:
#                 gerber_files.append(file)

#     for filename in gerber_files:
#         if "_NCDrill" in filename:
#             f = Path(filename)
#             print(os.getcwd() + "/" + f.stem + "." + old_replace_extension["_NCDrill"])
#             f.rename(os.getcwd() + "/" + f.stem + "." + old_replace_extension["_NCDrill"])
#         else:
#             f = Path(filename)
#             print(os.getcwd() + "/" + f.stem + "." + old_replace_extension["OtherGerber"])
#             f.rename(os.getcwd() + "/" + f.stem + "." + old_replace_extension["OtherGerber"])

print("{}件 完了しました。".format(len(gerber_files)))
print("[Enter]で終了します。")
input()