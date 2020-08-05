# Remove Duplicate Images

Remove the duplicate images between 2 selected folders. As it detects a duplicate  it moves it, from the first folder, in a folder in the same directory as the executable.

# Usage

When you start the application, first you are shown a folder dialog where you select the path to the first folder, then a second folder dialog where you select the second folder.
If you cancel any of the dialog an error window appears where you can press retry or terminate the app. After you selected the folders, it searches for the images, if none can be find, ar error window appears. As it iterate through images, it writes in the console when it deletes them. 

Suported image formats:

* Windows bitmaps - \*.bmp, \*.dib
* JPEG files - \*.jpeg, \*.jpg, \*.jpe
* JPEG 2000 files - \*.jp2
* Portable Network Graphics - \*.png
* WebP - \*.webp
* Portable image format - \*.pbm, \*.pgm, \*.ppm \*.pxm, \*.pnm
* PFM files - \*.pfm
* Sun rasters - \*.sr, \*.ras
* TIFF files - \*.tiff, \*.tif
* OpenEXR Image files - \*.exr
* Radiance HDR - \*.hdr, \*.pic
