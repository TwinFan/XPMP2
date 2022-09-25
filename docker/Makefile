platforms=win mac lin lin-bionic
container_focal=twinfan/focal-win-mac-lin-compile-env:02.00
container_bionic=twinfan/bionic-lin-compile-env:01.00

all: doc $(platforms)

.image_bionic:
	docker image pull $(container_bionic)
	docker inspect -f "{{ .ID}}" $(container_bionic) > .image_bionic

.image_focal:
	docker image pull $(container_focal)
	docker inspect -f "{{ .ID}}" $(container_focal) > .image_focal

bash_bionic: .image_bionic
	docker run -it --rm -v "$(realpath ..):/build" --entrypoint bash $(container_bionic)

bash_focal: .image_focal
	docker run -it --rm -v "$(realpath ..):/build" --entrypoint bash $(container_focal)

lin-bionic: .image_bionic
	docker run -i --rm -v "$(realpath ..):/build" --entrypoint docker/build.sh $(container_bionic) $@

lin win mac-arm mac-x86: .image_focal
	docker run -i --rm -v "$(realpath ..):/build" --entrypoint docker/build.sh $(container_focal) $@

mac: mac-arm mac-x86
	# Create universal binaries for XPMP2-Remote and XPMP2-Sample
	mkdir -p ../build-mac/mac_x64
	docker run -i --rm -v "$(realpath ..):/build" --entrypoint bash $(container_focal) -c 'lipo -create -output build-mac/mac_x64/XPMP2-Remote.xpl build-mac-x86/mac_x64/XPMP2-Remote.xpl build-mac-arm/mac_x64/XPMP2-Remote.xpl'
	docker run -i --rm -v "$(realpath ..):/build" --entrypoint bash $(container_focal) -c 'lipo -create -output build-mac/mac_x64/XPMP2-Sample.xpl build-mac-x86/mac_x64/XPMP2-Sample.xpl build-mac-arm/mac_x64/XPMP2-Sample.xpl'
	# Create universal binary for the XPMP2 library
	# (This is a bit ugly as it hard-codes the library version number as configured in CMakeList...but at least it will consequently fail when that version number is updated ;-)
	rm -r ../build-mac/XPMP2.framework || true
	cp -a ../build-mac-arm/XPMP2.framework ../build-mac/
	docker run -i --rm -v "$(realpath ..):/build" --entrypoint bash $(container_focal) -c 'lipo -create -output build-mac/XPMP2.framework/Versions/2.50/XPMP2 build-mac-x86/XPMP2.framework/Versions/2.50/XPMP2 build-mac-arm/XPMP2.framework/Versions/2.50/XPMP2'
	# Copy the universal library also to the XPMP2-Sample/lib folder
	cp ../build-mac/XPMP2.framework/Versions/2.50/XPMP2 ../XPMP2-Sample/lib/XPMP2.framework/Versions/2.50/XPMP2

doc:
	cd .. && /Applications/Doxygen.app/Contents/Resources/doxygen docs/XPMP2.doxygen

clean:
	rm -rf ../build-lin
	rm -rf ../build-lin-bionic
	rm -rf ../build-mac
	rm -rf ../build-mac-arm
	rm -rf ../build-mac-x86
	rm -rf ../build-win

.PHONY: clean doc $(platforms) bash_bionic bash_focal
