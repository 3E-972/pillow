import qbs.base 1.0

Application {
	files : ["simple.cpp"]
	Depends { name: "Qt"; submodules: ["core"] }
	Depends { name: "pillowcore" }
}

