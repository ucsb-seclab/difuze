//
// Created by machiry on 5/1/17.
//

#ifndef PROJECT_FILEUTILS_H
#define PROJECT_FILEUTILS_H

#include <string>

namespace IOCTL_CHECKER {
    class FileUtils {
    public:
        /***
         *  This function gets new relative path of the provided srcFilePath w.r.t to bitCodeDir
         *  For ex:
         *      if srcFilePath = <srcBasrDir>/a/b/c
         *      and bitCodeDir = /x/y
         *      and suffix = .includes
         *
         *      Then this function returns:
         *          /x/y/a/b/c.includes
         *
         * @param srcBaseDir The base directory which needs to be replaced with bitCodeDir
         * @param srcFilePath  The path of the source file.
         * @param bitCodeDir New directory path
         * @param suffix Suffix that needs to be used to return the new path.
         * @return
         */
        static std::string getNewRelativePath(std::string &srcBaseDir, std::string &srcFilePath,
                                              std::string &bitCodeDir, std::string &suffix);

    };
}

#endif //PROJECT_FILEUTILS_H
