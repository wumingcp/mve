#include "fasatomve.h"

#define THUMBNAIL_SIZE 50

FTM::FTM(const std::string & inputpath,const std::string & outputpath)
{
        this->input_path=inputpath;
        this->output_path=outputpath;
}

FTM::~FTM(){}

void FTM::loadFASA(){
    std::string path=this->input_path;
    if(!util::fs::dir_exists(path.c_str())){
		std::cerr << "[Error] path does not exist! Please check it!!"
			<< std::endl;
		exit(1);
	}

    std::string bundler = util::fs::join_path(path, "bundle.out");
    loadBundler(bundler);

    std::string list = util::fs::join_path(path, "list.txt");
    loadImages(list);

    loadDepthMap(path);
}


void FTM::writeMVE(){
    std::string path=this->output_path;
	if(!util::fs::dir_exists(path.c_str())){
		std::cerr << "[Error] path does not exist! Please check it!!"
			<< std::endl;
		exit(1);
	}
	/* new scene in the path*/
	/* Create destination directories. */
    std::cout << "[FTM] Creating output directories ..." << std::endl;
	const std::string output_path = util::fs::join_path(path, "scene");
    util::fs::mkdir(output_path.c_str());


	 /* Save bundle file. */
    std::cout << "[FTM] Saving bundle file ..." << std::endl;
    mve::save_photosynther_bundle(m_bundle, output_path + "/synth_0.out");

	std::cout << "[FTM] Saving view file ..." << std::endl;
	const std::string views_path = util::fs::join_path(output_path, "views");
	util::fs::mkdir(views_path.c_str());

	for (std::size_t i = 0; i < m_views.size(); ++i) {
		std::string fname = "/view_" + util::string::get_filled(i, 4) + ".mve";
		/* Save MVE file. */
        m_views[i]->save_mve_file_as(views_path + fname);
	}
}

template <class T>
typename mve::Image<T>::Ptr
limit_image_size (typename mve::Image<T>::Ptr img, int max_pixels)
{
    while (img->get_pixel_amount() > max_pixels)
        img = mve::image::rescale_half_size<uint8_t>(img);
    return img;
}


void FTM::loadBundler(const std::string& bundler){
	try{
		m_bundle = mve::load_bundler_bundle(bundler);
	}
	catch (std::exception& e)
    {
        std::cerr << "[Error] Cannot read bundle: " << e.what() << std::endl;
        std::exit(1);
    }
}

void
add_exif_to_view (mve::View::Ptr view, std::string const& exif)
{
    if (exif.empty())
        return;

    mve::ByteImage::Ptr exif_image = mve::ByteImage::create(exif.size(), 1, 1);
    std::copy(exif.begin(), exif.end(), exif_image->begin());
    view->add_data("exif", exif_image);
}


void FTM::loadImages(const std::string&  list){
	/* Read the list of original images filenames. */
    std::vector<std::string> files;

	/*
	 * Each camera in the bundle file corresponds to the ordered list of
	 * input images. Some cameras are set to zero, which means the input
	 * image was not registered. The paths of original images is required
	 * because Bundler does not compute the undistorted images.
	 */
	/*
     * The list of the original images is read from the list.txt file.
     */
    std::ifstream in(list.c_str());
    if (!in.good()){
        std::cerr << "[Error] Cannot read bundler list file " << list
			<< " !"<< std::endl;
        std::exit(1);
    }

    while (true) {
        std::string file, dummy;
        in >> file;
        std::getline(in, dummy);
        if (file.empty())
            break;
        files.push_back(file);
    }
    in.close();

	if (files.empty()){
		std::cerr << "[Error] Empty list of original images." << std::endl;
		std::exit(1);
	}
	if (files.size() != m_bundle->get_num_cameras()){
		std::cerr << "[Error] Invalid amount of original images." << std::endl;
		std::exit(1);
	}
	std::cout << "Recognized " << files.size()
		<< " original images from Noah's Bundler." << std::endl;

	const size_t num = files.size();
	m_views.resize(num);
    mve::Bundle::Cameras const& cams = m_bundle->get_cameras();
 	for(size_t i = 0 ; i < num ; ++i){
 		/*
         * For each camera in the bundle file, a new view is created.
         * Views are populated with ID, name, camera information,
         * undistorted RGB image, and optionally the original RGB image.
         */
        std::string fname = "view_" + util::string::get_filled(i, 4) + ".mve";

        /* Skip invalid cameras... */
        mve::CameraInfo cam = cams[i];
         if (cam.flen == 0.0f ){
            std::cerr << "  Skipping " << fname
                << ": Invalid camera." << std::endl;
            continue;
        }

        /* Extract name of view from original image or sequentially. */
        std::string view_name = util::fs::basename(files[i]);

        /* Fix issues with Noah Bundler camera specification. */

		/* Check focal length of camera, fix negative focal length. */
		if (cam.flen < 0.0f)
		{
			std::cout << "  Fixing focal length for " << fname << std::endl;
			cam.flen = -cam.flen;
			std::for_each(cam.rot, cam.rot + 9,
				math::algo::foreach_negate_value<float>);
			std::for_each(cam.trans, cam.trans + 3,
				math::algo::foreach_negate_value<float>);
 		}

		/* Convert from Noah Bundler camera conventions. */
		std::for_each(cam.rot + 3, cam.rot + 9,
			math::algo::foreach_negate_value<float>);
		std::for_each(cam.trans + 1, cam.trans + 3,
			math::algo::foreach_negate_value<float>);

		/* Check determinant of rotation matrix. */
		math::Matrix3f rmat(cam.rot);
		float rmatdet = math::matrix_determinant(rmat);
		if (rmatdet < 0.0f)
 		{
			std::cerr << "  Skipping " << fname
				<< ": Bad rotation matrix." << std::endl;
			continue;
		}

		 /* Create view and set headers. */
        m_views[i] = mve::View::create();
        m_views[i]->set_id(i);
        m_views[i]->set_name(view_name);
        m_views[i]->set_camera(cam);

        /* Load undistorted and original image, create thumbnail. */
        mve::ByteImage::Ptr original, undist, thumb;
        std::string exif;

		/* For Noah datasets, load original image and undistort it. */
		std::string orig_filename = files[i];
		original = load_8bit_image(orig_filename, &exif);
		thumb = create_thumbnail(original);

		/* Convert Bundler's focal length to MVE focal length. */
		cam.flen /= (float)std::max(original->width(), original->height());
		m_views[i]->set_camera(cam);

		if (cam.flen != 0.0f)
			undist = mve::image::image_undistort_bundler<uint8_t>
				(original, cam.flen, cam.dist[0], cam.dist[1]);

		//if (!import_original) original.reset();

		 /* Add images to view. */
        if (thumb != NULL)
            m_views[i]->add_image("thumbnail", thumb);

        if (undist != NULL)
        {
            undist = limit_image_size<uint8_t>(undist, std::numeric_limits<int>::max());
            m_views[i]->add_image("undistorted", undist);
          }
        else if (cam.flen != 0.0f && undist == NULL)
            std::cerr << "Warning: Undistorted image missing!" << std::endl;

        if (original != NULL)
            m_views[i]->add_image("original", original);
        if (original == NULL)
            std::cerr << "Warning: Original image missing!" << std::endl;

        /* Add EXIF data to view if available. */
        add_exif_to_view(m_views[i], exif);
	}
}

/* ---------------------------------------------------------------- */

mve::ByteImage::Ptr
FTM::load_8bit_image (std::string const& fname, std::string* exif)
{
    std::string lcfname(util::string::lowercase(fname));
    std::string ext4 = util::string::right(lcfname, 4);
    std::string ext5 = util::string::right(lcfname, 5);
    try
    {
        if (ext4 == ".jpg" || ext5 == ".jpeg")
            return mve::image::load_jpg_file(fname, exif);
        else if (ext4 == ".png" ||  ext4 == ".ppm"
            || ext4 == ".tif" || ext5 == ".tiff")
            return mve::image::load_file(fname);
    }
    catch (...)
    { }

    return mve::ByteImage::Ptr();
}


template <typename T>
void
find_min_max_percentile (typename mve::Image<T>::ConstPtr image,
    T* vmin, T* vmax)
{
    typename mve::Image<T>::Ptr copy = mve::Image<T>::create(*image);
    std::sort(copy->begin(), copy->end());
    *vmin = copy->at(copy->get_value_amount() / 10);
    *vmax = copy->at(9 * copy->get_value_amount() / 10);
}


mve::ByteImage::Ptr
FTM::create_thumbnail (mve::ImageBase::ConstPtr img)
{
    mve::ByteImage::Ptr image;
    switch (img->get_type())
    {
        case mve::IMAGE_TYPE_UINT8:
            image = mve::image::create_thumbnail<uint8_t>
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            break;

        case mve::IMAGE_TYPE_UINT16:
        {
            mve::RawImage::Ptr temp = mve::image::create_thumbnail<uint16_t>
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            uint16_t vmin, vmax;
            find_min_max_percentile(temp, &vmin, &vmax);
            image = mve::image::raw_to_byte_image(temp, vmin, vmax);
            break;
        }

        case mve::IMAGE_TYPE_FLOAT:
        {
            mve::FloatImage::Ptr temp = mve::image::create_thumbnail<float>
                (img, THUMBNAIL_SIZE, THUMBNAIL_SIZE);
            float vmin, vmax;
            find_min_max_percentile(temp, &vmin, &vmax);
            image = mve::image::float_to_byte_image(temp, vmin, vmax);
            break;
        }

        default:
            return mve::ByteImage::Ptr();
    }

    return image;
}



void FTM::loadDepthMap(const std::string& path){
    if(!util::fs::dir_exists(path.c_str())){
        std::cerr << "[Error] This directory does not exist! Please check "
            << "this directory " << path << std::endl;
        std::exit(1);
    }

    util::fs::Directory depthmap_dir;
    depthmap_dir.scan(path);

    //Filter depth maps
    std::vector<std::string> depthmapfiles;
    for(size_t i = 0 ; i < depthmap_dir.size() ; ++i){
        const size_t length = depthmap_dir[i].name.length();
        if(depthmap_dir[i].name.substr(0, 8) == "depthMap"
            && depthmap_dir[i].name.substr(length - 3) == "txt"){
            depthmapfiles.push_back(depthmap_dir[i].name);
        }
    }
    std::cout << "[FTM] Detected " <<  depthmapfiles.size() << " depth maps" << std::endl;

    for(size_t i = 0 ; i < depthmapfiles.size() ; ++i){
        const int last = depthmapfiles[i].rfind('.');
        const int front = depthmapfiles[i].rfind('_');
        int index = atoi(depthmapfiles[i].substr(front + 1, last - front - 1).c_str());
        mve::View::Ptr view = m_views[index];

        int width = 0 , height = 0 , WType = 0;
        std::ifstream fin(util::fs::join_path(path, depthmapfiles[i]).c_str());
    	if( !fin.good() ){
            std::cerr << "[Error] Cannot read this file." << std::endl;
            exit(1);
    	}
        std::string headTmp;
    	getline(fin, headTmp);
    	int flag=1;
    	for(int j = 0 ; j < (int)headTmp.size() ; ++j)
    	{
    	    if(flag == 1 && !isdigit(headTmp[j]))
    	        continue;
    	    while(isdigit(headTmp[j]) && 1 == flag){
    	        height = height * 10 + headTmp[j] - '0';
    	        ++j;
    	        if(!isdigit(headTmp[j]))
    	            flag = 2;
    	    }

    	    while(isdigit(headTmp[j]) && 2 == flag){
    	        width = width * 10 + headTmp[j] - '0';
    	        ++j;
    	        if(!isdigit(headTmp[j]))
    	            flag = 3;
    	    }

    	    while(isdigit(headTmp[j]) && 3 == flag){
    	        WType = WType * 10 + headTmp[j] - '0';
    	        ++j;
    	    }
    	}

        mve::FloatImage::Ptr depthmap = mve::FloatImage::create(width, height, 1);
        if(0 == WType){
            for(int row = 0 ; row < height ; ++row){
                for(int col = 0 ; col < width ; ++col){
                    size_t index2 = row * width + col;
                    fin >> depthmap->at(index2);
                }
            }
        }
        else if(1 == WType){
            fin.close();
            fin.open(util::fs::join_path(path, depthmapfiles[i]).c_str(),
                          std::ios::binary);
            getline(fin, headTmp);
            for(int row = 0 ; row < height ; ++row){
                for(int col = 0 ; col < width ; ++col){
                    size_t index2 = row * width + col;
                    float tmp;
                    fin.read(reinterpret_cast<char*>(&tmp), sizeof(float));
                    depthmap->at(index2) = tmp;
                }
            }
        }

        fin.close();

        view->set_image("depth-L0", depthmap);
        mve::ByteImage::Ptr undist = view->get_image("undistorted");
        view->add_image("undist-L0", undist);
    }
}
