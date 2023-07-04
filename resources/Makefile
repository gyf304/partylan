.PHONY: all clean

all: steam/generated/460x215.png steam/generated/616x353.png \
	steam/generated/231x87.png steam/generated/374x448.png \
	steam/generated/600x900.png steam/generated/hero-3840x1240.png \
	steam/generated/logo-1280x720.png \
	windows/icon.ico

clean:
	rm -rf steam/generated
	rm -rf windows/generated

steam/generated:
	mkdir -p steam/generated

steam/generated/460x215.png: steam/generated common/images/background.png common/images/title.png
	convert common/images/background.png -gravity SouthWest -crop 460x215+0+0 +repage $@.background.png
	convert common/images/title.png -resize 460x215\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

steam/generated/616x353.png: steam/generated common/images/background.png common/images/title.png
	convert common/images/background.png -gravity SouthWest -crop 616x353+0+0 +repage $@.background.png
	convert common/images/title.png -resize 616x353\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

steam/generated/231x87.png: steam/generated common/images/background.png common/images/title.png
	convert common/images/background.png -gravity SouthWest -crop 231x87+0+0 +repage $@.background.png
	convert common/images/title.png -resize 231x87\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

steam/generated/374x448.png: steam/generated common/images/background.png common/images/title.png
	convert common/images/background.png -gravity SouthWest -crop 374x448+0+0 +repage $@.background.png
	convert common/images/title.png -resize 374x448\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

steam/generated/600x900.png: steam/generated common/images/background.png common/images/title.png
	convert common/images/background.png -gravity SouthWest -crop 600x900+0+0 +repage $@.background.png
	convert common/images/title.png -resize 600x900\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

steam/generated/hero-3840x1240.png: steam/generated common/images/background.png common/images/title.png
	convert -size 3840x3840 tile:common/images/background.png -gravity SouthWest -crop 3840x1240+0+0 +repage $@

steam/generated/logo-1280x720.png: steam/generated common/images/title.png
	convert common/images/title.png -resize 1280x720\> -gravity SouthWest -background transparent -extent 1280x720 $@

windows/generated:
	mkdir -p windows/generated

windows/generated/icon-%-dark-mode.png: windows/generated common/images/icon.png
	convert common/images/icon.png -resize $*\> $@

windows/generated/icon-%-light-mode.png: windows/generated common/images/icon.png
	convert common/images/icon.png -resize $*\> -negate $@

windows/generated/icon-light-mode.ico: windows/generated/icon-16x16-light-mode.png windows/generated/icon-32x32-light-mode.png windows/generated/icon-48x48-light-mode.png windows/generated/icon-256x256-light-mode.png
	convert $^ -colors 256 $@

windows/generated/icon-dark-mode.ico: windows/generated/icon-16x16-dark-mode.png windows/generated/icon-32x32-dark-mode.png windows/generated/icon-48x48-dark-mode.png windows/generated/icon-256x256-dark-mode.png
	convert $^ -colors 256 $@

windows/icon.ico: windows/generated/icon-dark-mode.ico
	cp $< $@
