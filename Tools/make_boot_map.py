"""Create the empty Boot level the UI-driven game runs in."""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not unreal.EditorAssetLibrary.does_asset_exist("/Game/BFK/Maps/Boot"):
    les.new_level("/Game/BFK/Maps/Boot")
    unreal.log("BOOT_CREATED")
else:
    unreal.log("BOOT_EXISTS")
les.save_all_dirty_levels()
