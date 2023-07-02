.PHONY: all clean

all: generated/460x215.png generated/616x353.png generated/231x87.png generated/374x448.png generated/600x900.png generated/hero-3840x1240.png generated/logo-1280x720.png

clean:
	rm -rf generated

generated:
	mkdir -p generated

generated/460x215.png: generated background.png title.png
	convert background.png -gravity SouthWest -crop 460x215+0+0 +repage $@.background.png
	convert title.png -resize 460x215\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

generated/616x353.png: generated background.png title.png
	convert background.png -gravity SouthWest -crop 616x353+0+0 +repage $@.background.png
	convert title.png -resize 616x353\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

generated/231x87.png: generated background.png title.png
	convert background.png -gravity SouthWest -crop 231x87+0+0 +repage $@.background.png
	convert title.png -resize 231x87\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

generated/374x448.png: generated background.png title.png
	convert background.png -gravity SouthWest -crop 374x448+0+0 +repage $@.background.png
	convert title.png -resize 374x448\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

generated/600x900.png: generated background.png title.png
	convert background.png -gravity SouthWest -crop 600x900+0+0 +repage $@.background.png
	convert title.png -resize 600x900\> $@.title.png
	convert $@.background.png $@.title.png -gravity SouthWest -composite $@
	rm $@.background.png $@.title.png

generated/hero-3840x1240.png: generated background.png title.png
	convert -size 3840x3840 tile:background.png -gravity SouthWest -crop 3840x1240+0+0 +repage $@

generated/logo-1280x720.png: generated title.png
	convert title.png -resize 1280x720\> -gravity SouthWest -background transparent -extent 1280x720 $@
