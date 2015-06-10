#include "fasatomve.h"

struct AppSettings
{
  std::string loadFASA_path;
  std::string writeMVE_path;
};

int main(int argc, char** argv){

    if(argc<3)
    {
       std::cerr << "Input param less 2, please input loadFASA_path and writeMVE_path" 
		 << std::endl << std::endl
	 	 << "Usage : " << argv[0] << " loadFASA_path writeMVE_path" << std::endl
		 << "eg: ./FASAtoMVE et/ ~/Desktop/" << std::endl << std::endl;
       return 1;
    }

    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT LOADFASA_PATH  OUTOUT WRITEMVE_PATH ");
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_exit_on_error(true);
    args.parse(argc, argv);

    AppSettings app;

    app.loadFASA_path=args.get_nth_nonopt(0);
    app.writeMVE_path=args.get_nth_nonopt(1);

    //std::cout<<argc<<" input path is:  "<<app.loadFASA_path<<" output path is :"<<app.writeMVE_path<<std::endl;

    FTM ftm(app.loadFASA_path, app.writeMVE_path);
    ftm.loadFASA();
    ftm.writeMVE();
    return 0;
}
