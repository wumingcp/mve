#include "mvetofasa.h"
#include "mve/image_io.h"

MTF::MTF(const std::string& inputpath, const std::string& outputpath){
    this->input_path = inputpath;
    this->output_path = outputpath;
}

MTF::~MTF(){}

void MTF::loadMVE(){
    std::cout << "[MTF] Loading scene ..." << std::endl;
    m_scene = mve::Scene::create(input_path);
}

void MTF::writeFASA(){
   if(!util::fs::dir_exists(output_path.c_str())){
        std::cout << "[MTF] Create output directory ..." << std::endl;
        util::fs::mkdir(output_path.c_str());
   }

    writeImages(output_path);

    std::string bundlerfile = util::fs::join_path(output_path, "bundle.out");
    writeBundler(bundlerfile);

    writeDepthMaps(output_path);
}

void MTF::writeDepthMaps(const std::string& path){
    std::cout << "[MTF] Writing depth maps ..." << std::endl;
    mve::Scene::ViewList& views = m_scene->get_views();
    for(size_t i = 0 ; i < views.size() ; ++i){
        mve::View::Ptr view = views[i];
        mve::FloatImage::Ptr image = view->get_float_image("depth-L0");
	const int height = image->height();
	const int width = image->width();

	std::string depthmapname = "depthMap_"
        + util::string::get_filled(i, 4) + ".txt";
	depthmapname = util::fs::join_path(path, depthmapname);

	std::ofstream out(depthmapname.c_str());
	out << height << " " << width << " 1\n";
	for(int row = 0 ; row < height ; ++row){
		for(int col = 0 ; col < width ; ++col){
			const int index = row * width + col;
			out << image->at(index) << " ";
		}
		out << std::endl;
	}
	out.close();
    }
}

void MTF::writeBundler(const std::string& bundlerfile){
    std::cout << "[MTF] Writing bundle ..." << std::endl;
    mve::Bundle::ConstPtr bundle = m_scene->get_bundle();
    mve::save_photosynther_bundle(bundle, bundlerfile);
}

void MTF::writeImages(const std::string& imagespath){
    std::cout << "[MTF] Writing images ..." << std::endl;
    util::fs::mkdir(util::fs::join_path(imagespath,"images").c_str());

    mve::Scene::ViewList& views = m_scene->get_views();
    std::vector<std::string> lists(views.size());
    for(size_t i = 0 ; i < views.size() ; ++i){
        mve::View::Ptr view = views[i];
        lists[i] = util::fs::join_path(imagespath,"images");
        lists[i] = util::fs::join_path(lists[i], view->get_name());
        mve::ByteImage::Ptr image = view->get_image("original");
        mve::image::save_file(image, lists[i]);
    }

    std::string list = util::fs::join_path(imagespath, "list.txt");
    std::ofstream out(list.c_str());
    for(size_t i = 0 ; i < lists.size() ; ++i){
        out << lists[i] << std::endl;
    }
    out.close();
}
