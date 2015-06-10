#include "mvetofasa.h"

struct AppSettings
{
  std::string loadMVE_path;
  std::string writeFASA_path;
}; 

int  main(int argc, char** argv){

    if(argc<3)
    { 
       std::cerr << "Input param less 2, please input loadMVE_path and writeFASA_path" 
		 << std::endl << std::endl
	 	 << "Usage : " << argv[0] << " loadFMVE_path writeFASA_path" << std::endl
		 << "eg: ./FASAtoMVE scene et/" << std::endl << std::endl;
       return 1;
    }

    util::Arguments args;
    args.set_usage(argv[0], "[ OPTIONS ] INPUT LOADMVE_PATH  OUTOUT WRITEFASA_PATH ");
    args.set_nonopt_minnum(2);
    args.set_nonopt_maxnum(2);
    args.set_exit_on_error(true);
    args.parse(argc, argv);

    AppSettings app;

    app.loadMVE_path=args.get_nth_nonopt(0);
    app.writeFASA_path=args.get_nth_nonopt(1);

    //std::cout<<argc<<" input path is:  "<<app.loadFASA_path<<" output path is :"<<app.writeMVE_path<<std::endl;

    MTF mtf(app.loadMVE_path, app.writeFASA_path);
    mtf.loadMVE();
    mtf.writeFASA();
    return 0;
}
