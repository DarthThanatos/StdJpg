from pylab import *
from matplotlib.pyplot import *
import numpy 
import sys

def showPicture(bitmap):
	imshow(bitmap, cmap = plt.get_cmap('gray'))
	show()

def display(jpg,width,height):
	output = []
	for i in range(height):
		output.append([])
		for j in range(width):
			pixel = []
			for k in range(3):
				pixel.append(uint8(jpg[k][i][j]))
			output[i].append(pixel)
	showPicture(output)

def main():
	compared_image_path = None
	try:
		compared_image_path = sys.argv[1]
	except IndexError:
		print("Usage: python display_jpg.py original.jpg")
		sys.exit(0)
	filename = "result.txt"
	with open(filename, "rb") as f:
		jpg_desc = f.read().split()
		width,height = int(jpg_desc[0]), int(jpg_desc[1])
		jpg = []
		for k in range(3):
			jpg.append([])
			for i in range(height):
				jpg[k].append([])
				for j in range (width):
					jpg[k][i] += [jpg_desc[2 + k*width*height + (i*width + j)]]
		display(jpg,width,height)
	showPicture(imread(compared_image_path))

main()