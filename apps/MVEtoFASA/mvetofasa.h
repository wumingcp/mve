/*************************************************************************
	> File Name: fasatomve.h
	> Author: Cui Peng
	> Mail: cuipeng300@sina.com
	> Created Time: 2015/6/3 11:51
************************************************************************/

#ifndef _FASATOMVE_H
#define _FASATOMVE_H

#include <cstring>
#include <cstdlib>
#include <string>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

#include "math/matrix.h"
#include "math/matrix_tools.h"
#include "math/algo.h"
#include "util/arguments.h"
#include "util/string.h"
#include "util/file_system.h"
#include "util/tokenizer.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"
#include "mve/view.h"
#include "mve/image.h"
#include "mve/image_tools.h"
#include "mve/image_io.h"
#include "mve/image_exif.h"
#include "mve/mesh.h"
#include "mve/mesh_tools.h"

class MTF{
    public:
    MTF(const std::string & inputpath,const std::string & outputpath);
    ~MTF();

    /**
     * @brief loading FASA data
     * @param path The path where FASA data locates
     */
    void loadMVE();

    /**
     * @brief write data to disk using MVE format
     * @param path The location where we want to save the scene
     */
    void writeFASA();

    protected:

	/**
	* @brief loading images according to the list
	* @param list the list for images
	*/
	void writeImages(const std::string& list);

	/**
	* @brief loading bundler data
	* @param bundlerfile bundler file for saving bundler data
	*/
	void writeBundler(const std::string& bundler);

	/**
	* @brief loading multiple depth-maps for each image
	* @param path The path where we save these depth-maps
	*/
	void writeDepthMap(const std::string& path);


    private:

    mve::scene::Ptr m_scene;
	std::string input_path;
	std::string output_path;

};

#endif
