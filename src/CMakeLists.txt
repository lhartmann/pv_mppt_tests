INCLUDE_DIRECTORIES(${MPPT_SOURCE_DIR}/src)

ADD_EXECUTABLE(mppt
	mppt.cpp
	mppt_inccond.h mppt_mlam.cpp mppt_mlamhf.h bilinear.cpp
	debug.cpp arg_tool.cpp straux.cpp progressbar.cpp error.cpp
	kepco.cpp serial.cpp
	pvgen.cpp pvgen_sc.cpp pvgen_mc.cpp pvgen_mpp_I.cpp pvgen_models.cpp pvgen_model_test.cpp
	denis_sensors.cpp
)
TARGET_LINK_LIBRARIES(mppt rt pthread)

ADD_EXECUTABLE(dat2mat dat2mat progressbar straux debug)

ADD_EXECUTABLE(gentbl
	gentbl.cpp
	pvgen.cpp pvgen_sc.cpp pvgen_mc.cpp
	mppt_mlam.cpp bilinear.cpp
	debug.cpp error.cpp
)

ADD_EXECUTABLE(genstim
	genstim.cpp
	pvgen.cpp pvgen_sc.cpp pvgen_mc.cpp
	debug.cpp error.cpp iniloader.cpp straux.cpp regexpp.cpp
)

ADD_EXECUTABLE(stim2sas
	stim2sas.cpp
	debug.cpp arg_tool.cpp straux.cpp progressbar.cpp regexpp.cpp error.cpp
	pvgen.cpp pvgen_sc.cpp pvgen_mpp_I.cpp pvgen_models.cpp
)
