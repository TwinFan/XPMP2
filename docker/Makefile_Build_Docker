platforms=win mac lin lin-bionic
container_focal=focal-win-mac-lin-compile-env
container_bionic=bionic-lin-compile-env

all: doc $(platforms)

.image_bionic: Dockerfile_Bionic build.sh
	docker build . -t $(container_bionic) -f Dockerfile_Bionic
	docker inspect -f "{{ .ID}}" $(container_bionic):latest > .image_bionic

.image_focal: Dockerfile_Focal build.sh
	docker build . -t $(container_focal) -f Dockerfile_Focal
	docker inspect -f "{{ .ID}}" $(container_focal):latest > .image_focal

bash_bionic: .image_bionic
	docker run -it --rm -v "$(realpath ..):/build" --entrypoint bash $(container_bionic):latest

bash_focal: .image_focal
	docker run -it --rm -v "$(realpath ..):/build" --entrypoint bash $(container_focal):latest

lin-bionic: .image_bionic
	docker run -i --rm -v "$(realpath ..):/build" $(container_bionic):latest $@

lin win mac: .image_focal
	docker run -i --rm -v "$(realpath ..):/build" $(container_focal):latest $@

doc:
	cd .. && /Applications/Doxygen.app/Contents/Resources/doxygen docs/XPMP2.doxygen

.PHONY: doc $(platforms) bash_bionic bash_focal
