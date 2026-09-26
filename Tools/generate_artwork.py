"""Generate production background/dial artwork (not a flattened reference image)."""
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'Design'/'Assets';out.mkdir(exist_ok=True)
w,h=1536,922
y,x=np.mgrid[:h,:w].astype(float)
a=np.empty((h,w,3),float);a[:]=[1,10,20]
a += ((1-y/h)**2)[:,:,None]*np.array([1.5,8,12])[None,None,:]
def haze(cx,cy,sx,sy,rgb):
 global a
 p=np.exp(-((x-cx)/sx)**2-((y-cy)/sy)**2)
 a+=p[:,:,None]*np.array(rgb)[None,None,:]
haze(480,250,570,360,[2,10,17]);haze(360,625,360,290,[1,8,15])
haze(1120,320,450,360,[1,13,22]);haze(775,56,260,110,[1,9,16])
haze(850,720,460,275,[0,6,13])
vignette=np.clip(1-.23*((x-768)/768)**2-.18*((y-430)/500)**2,.56,1)
a*=vignette[:,:,None]
a[:104]*=.80
Image.fromarray(np.uint8(np.clip(a,0,255))).save(out/'background.png')
# Transparent face at 3x source resolution: subtle shaded rim and concave light.
n=384;y,x=np.mgrid[:n,:n].astype(float);u=(x-(n-1)/2)/3;v=(y-(n-1)/2)/3
r=np.hypot(u,v);rgba=np.zeros((n,n,4),float)
alpha=np.clip(53.7-r,0,1)
light=np.exp(-((u+19)/43)**2-((v+23)/39)**2)
edge=np.exp(-((r-51.5)/1.15)**2)*(0.38+0.62*np.clip((-u-v)/90,0,1))
rgb=np.array([0,8,16])[None,None,:]+light[:,:,None]*np.array([11,21,29])[None,None,:]
rgb+=edge[:,:,None]*np.array([8,17,22])[None,None,:]
rgba[:,:,:3]=rgb;rgba[:,:,3]=alpha*255
Image.fromarray(np.uint8(np.clip(rgba,0,255))).save(out/'dial-face.png')
print(out)
