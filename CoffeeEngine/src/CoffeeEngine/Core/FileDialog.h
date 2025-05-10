/**
 * @defgroup core Core
 * @brief Core components of the CoffeeEngine.
 * @{
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace Coffee {

    /**
     * @brief Arguments for the file dialog.
     */
    struct FileDialogArgs {
        /**
         * @brief Filter for file types.
         */
        struct Filter {
            const char* Name;      /**< Name of the filter. */
            const char* Extension; /**< Extension of the filter. */
        };
        std::vector<Filter> Filters = {}; /**< List of filters. */
        std::string DefaultPath = "";     /**< Default path for the file dialog. */
        std::string DefaultName = "";     /**< Default name for the file dialog. */
    };

    /**
     * @brief Class for handling file dialogs.
     */
    class FileDialog {
    public:
        /**
         * @brief Opens a file dialog to select a file.
         * @param args Arguments for the file dialog.
         * @return The selected file path.
         */
        static const std::filesystem::path OpenFile(const FileDialogArgs& args = {});

        /**
         * @brief Opens a file dialog to save a file.
         * @param args Arguments for the file dialog.
         * @return The selected file path.
         */
        static const std::filesystem::path SaveFile(const FileDialogArgs& args = {});

        /**
         * @brief Opens a file dialog to select a directory.
         *
         * Note that the only arguments used is DefaultPath
         *
         * @return The selected directory path
         */
        static const std::filesystem::path PickFolder(const FileDialogArgs& args = {});
    };

    /** @} */

} // namespace Coffee

