#!/bin/bash

IMAGE_SOURCE=$1
IMAGE_GRAYSCALE=images/grayscale.raw
IMAGE_GRAYS_JPEG=images/grayscale.jpeg
IMAGE_MEAN=images/mean.raw
IMAGE_MEAN_JPEG=images/mean.jpeg

if [ ! $IMAGE_SOURCE ]
then
    echo "Usage: $0 <jpeg_file>"
    exit -1
fi

echo "Making grayscale image..."
JPEG2RAW=`./build/jpeg2raw $IMAGE_SOURCE $IMAGE_GRAYSCALE`
if [ $? != 0 ]
then
    echo $JPEG2RAW
    echo "Aborting..."
    exit -1
fi

JPEG2RAW=`echo $JPEG2RAW | grep pixels`
WIDTH=`echo $JPEG2RAW | awk '{print $5}'`
HEIGHT=`echo $JPEG2RAW | awk '{print $7}'`
echo "Width is $WIDTH"
echo "Height is $HEIGHT"

echo "Saving to jpeg file..."
./build/raw2jpeg $IMAGE_GRAYSCALE $IMAGE_GRAYS_JPEG $WIDTH $HEIGHT
if [ $? != 0 ]
then
    echo "Aborting..."
    exit -1
fi

echo "Applying mean filter to grayscale image..."
./build/mean_filter_2d $IMAGE_GRAYSCALE $IMAGE_MEAN $WIDTH $HEIGHT
if [ $? != 0 ]
then
    echo "Aborting..."
    exit -1
fi

echo "Saving to jpeg file..."
./build/raw2jpeg $IMAGE_MEAN $IMAGE_MEAN_JPEG $WIDTH $HEIGHT
if [ $? != 0 ]
then
    echo "Aborting..."
    exit -1
fi

echo "Done."

exit 0
