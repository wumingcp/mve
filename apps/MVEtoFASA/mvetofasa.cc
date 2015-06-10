#include "mvetofasa.h"

MTF::MTF(const std::string& inputpath, const std::string& outputpath){
    this->input_path = inputpath;
    this->output_path = outputpath;
}

MTF::~MTF(){}

void MTF::loadScene(){
    std::cout << "[MTF] Loading scene ..." << std::endl;
    m_scene = mve::Scene::create(input_path);
}

void MTF::writeFASA(){
    writeImages(output_path);

    std::string bundlerfile = util::fs::join_path(output_path, "bundle.out");
    writeBundler(bundlerfile);

    writeDepthMaps(output_path);
}

void MTF::DepthMaps(const std::string& path){
    mve::ViewList& views = m_scene->get_views();
    for(size_t i = 0 ; i < views.size() ; ++i){
        mve::View::Ptr view = views[i];
        mve::ImageBase::Ptr image = view->get_image("depth-L0");
	const int height = image->height();
	const int width = image->width();

	std::string depthmapname = std::string("depthMap_") + i + std::string(".txt");
	depthmapname = util::fs::join_path(path, depthmapname);

	std::ostream out(depthmapname.c_str());
	out << height << " " << width << " 1\n";
	for(int row = 0 ; row < height ; ++row){
		for(int col = 0 ; col < width ; ++col){
			const int index = row * width + col;
			out << image->at(index) << " ";
		}
		cout << std::endl;
	}
	out.close();
    }
}

void MTF::writeBundler(const std::string& bundlerfile){
    mve::Bundler::Ptr bundle = m_views->get_bundle();

    mve::save_noah_bundler(bundle, bundlerfile);
}

void MTF::writeImages(const std::string& imagespath){
    mve::ViewList& views = m_scene->get_views();
    std::vector<std::string> lists(views.size());
    for(size_t i = 0 ; i < views.size() ; ++i){
        mve::View::Ptr view = ViewList[i];
        lists[i] = util::fs::join_path(imagespath,"images");
        lists[i] = util::fs::join_path(lists[i], view->get_filename());
        mve::FloatImage::Ptr image = view->get_float_image("original");
        image->save(lists[i]);
    }

    std::string list = util::fs::join_path(imagespath, "list.txt");
    std::ostream out(list.c_str());
    for(size_t i = 0 ; i < lists.size() ; ++i){
        out << lists[i] << std::endl;
    }
    out.close();
}
