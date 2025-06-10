#include "FileDialog.h"
#include "CoffeeEngine/Core/Log.h"
#include <nfd.h>

namespace Coffee {

    const std::filesystem::path FileDialog::OpenFile(const FileDialogArgs& args)
    {
        NFD_Init();

        nfdu8char_t *outPath = nullptr;

        /* nfdu8filteritem_t filters[2] = { { "Text Files", "txt" }, { "C++ Files", "cpp" } };
        nfdopendialogu8args_t args = {0};
        args.filterList = filters;
        args.filterCount = 2; */

        nfdopendialogu8args_t nfdArgs = {0};
        nfdArgs.filterCount = args.Filters.size();
        nfdArgs.filterList = (nfdu8filteritem_t*)args.Filters.data();
        nfdArgs.defaultPath = args.DefaultPath.c_str();

        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &nfdArgs);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path(outPath);
            NFD_FreePathU8(outPath);
            NFD_Quit();
            return path;
        }
        else if (result == NFD_CANCEL)
        {
            COFFEE_INFO("User pressed cancel.");
            NFD_Quit();
            return "";
        }
        else
        {
            COFFEE_ERROR("Error: %s\n", NFD_GetError());
            NFD_Quit();
            return "";
        }

    }

    const std::filesystem::path FileDialog::SaveFile(const FileDialogArgs& args)
    {
        NFD_Init();

        nfdchar_t* outPath = nullptr;

        nfdsavedialogu8args_t nfdArgs = {0};
        nfdArgs.filterCount = args.Filters.size();
        nfdArgs.filterList = (nfdu8filteritem_t*)args.Filters.data();
        nfdArgs.defaultPath = args.DefaultPath.c_str();
        nfdArgs.defaultName = args.DefaultName.c_str();

        nfdresult_t result = NFD_SaveDialogU8_With(&outPath, &nfdArgs);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path(outPath);
            NFD_FreePathU8(outPath);
            NFD_Quit();
            return path;
        }
        else if (result == NFD_CANCEL)
        {
            COFFEE_INFO("User pressed cancel.");
            NFD_Quit();
            return "";
        }
        else
        {
            COFFEE_ERROR("Error: %s\n", NFD_GetError());
            NFD_Quit();
            return "";
        }

    }
    const std::filesystem::path FileDialog::PickFolder(const FileDialogArgs& args)
    {
        NFD_Init();

        nfdu8char_t* outPath;

        // show the dialog
        nfdresult_t result = NFD_PickFolder(&outPath, nullptr);
        if (result == NFD_OKAY)
        {
            std::filesystem::path path(outPath);
            NFD_FreePathU8(outPath);
            NFD_Quit();
            return path;
        }
        else if (result == NFD_CANCEL)
        {
            COFFEE_INFO("User pressed cancel.");
            NFD_Quit();
            return "";
        }
        else
        {
            COFFEE_ERROR("Error: %s\n", NFD_GetError());
            NFD_Quit();
            return "";
        }

    }

} // namespace Coffee