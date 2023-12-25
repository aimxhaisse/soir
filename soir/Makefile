# Makefile to build & setup soir.

build_type := Release
data_dir := ../data/waves
daw_assets := https://assets.maethstro.com/waves/daw_data.tar.gz
juce := ./src/build/JUCE
build := ./src/build/waves
bin := ./bin
juce_build := ./src/build/build-juce

all: $(juce) $(build)

deps: $(data_dir) $(data_dir)/assets.tar.gz

juce: $(juce)

$(data_dir)/assets.tar.gz:
	wget $(daw_assets) -O $(data_dir)/assets.tar.gz
	cd $(data_dir) && tar -xf assets.tar.gz	

$(data_dir):
	mkdir -p $(data_dir)

$(juce):
	git clone https://github.com/juce-framework/JUCE $(juce)

$(build):
	mkdir -p $(build) $(bin)
	cd $(build) && cmake ../../ -DCMAKE_BUILD_TYPE=$(build_type) && make -j16
	cp src/build/waves/Waves_artefacts/$(build_type)/Waves $(bin)/waves

fmt:
	clang-format -i src/*.cpp src/*.hh

build: $(build)

clean:
	sudo rm -rf $(juce) $(juce_build) $(build)

fclean: clean dclean

dclean:
	sudo rm -rf $(data_dir)

.PHONY: all $(build) build fmt deps dclean fclean juce
