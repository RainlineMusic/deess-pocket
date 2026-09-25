from PIL import Image,ImageDraw,ImageFont,ImageFilter
from pathlib import Path
import numpy as np, math
R=Path(__file__).resolve().parents[1]; assets=R/'Design'/'Assets'
im=Image.open(assets/'background.png').convert('RGBA'); d=ImageDraw.Draw(im)
def font(n): return ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',n)
def spaced(text,spacing=8): return (' '*max(1,spacing//4)).join(text)
pale=(205,219,226,230); grid=(22,48,62,120)
# header
d.line((0,103,1536,103),fill=(7,28,43,255),width=1)
for yy in [33,41,49]:d.line((37,yy,64,yy),fill=(105,125,136,190),width=1)
d.text((768,14),'D E E S S',font=font(34),anchor='ma',fill=(158,179,194,235))
d.text((768,60),'P O C K E T   S E R I E S',font=font(10),anchor='ma',fill=(134,153,165,220))
d.line((636,65,674,65),fill=(57,81,96,170));d.line((862,65,900,65),fill=(57,81,96,170))
d.ellipse((1465,26,1506,67),outline=(84,107,121,210),width=1);d.arc((1476,36,1495,55),-220,40,fill=(130,150,162,220),width=2);d.line((1485,32,1485,46),fill=(130,150,162,220),width=2)
left,right=36,1480
def fx(f):return left+math.log(f/20,1000)*(right-left)
for f in [20,50,100,200,500,1000,2000,5000,10000,20000]:
 x=fx(f);d.line((x,104,x,862),fill=grid,width=1);label=f'{int(f/1000)}k' if f>=1000 else str(f);d.text((x,884),label,font=font(14),anchor='mm',fill=(205,211,214,235))
for db in range(0,-61,-6):
 y=162-db*9;d.line((15,y,1482,y),fill=(35,62,76,115),width=1);d.text((1488,y),str(db)+(' dB' if db==0 else ''),font=font(13),anchor='lm',fill=(171,185,194,235))
# plausible analyzer landscape
freq=np.geomspace(20,20000,420); log=np.log10(freq);spec=-72+34*np.exp(-((log-2.55)/.58)**2)-11*np.maximum(log-3.85,0)
rng=np.random.default_rng(4);spec+=rng.normal(0,2.5,len(freq))*np.clip((log-1.4),0,1)
y=np.clip(812-(spec+90)*650/90,145,862);pts=[(fx(f),v) for f,v in zip(freq,y)]
poly=pts+[(right,862),(left,862)]
fill=Image.new('RGBA',im.size);fd=ImageDraw.Draw(fill);fd.polygon(poly,fill=(174,188,197,65));fd.line(pts,fill=(197,209,216,150),width=1);im=Image.alpha_composite(im,fill);d=ImageDraw.Draw(im)
# response curves
for color,depth,start in [((0,220,250,230),11,3500),((255,207,0,240),11,2300),((244,0,235,240),18,1850)]:
 q=[]
 for f in freq:
  red=depth/(1+math.exp(-5*(math.log2(f/start))))
  if color[0]>200 and color[2]>200:red+=6*math.exp(-((math.log(f/3200)/.13)**2))+7*math.exp(-((math.log(f/4800)/.12)**2))+6*math.exp(-((math.log(f/8200)/.1)**2))
  q.append((fx(f),162+red*9))
 glow=Image.new('RGBA',im.size);gd=ImageDraw.Draw(glow);gd.line(q,fill=color[:-1]+(90,),width=10);glow=glow.filter(ImageFilter.GaussianBlur(9));im=Image.alpha_composite(im,glow);d=ImageDraw.Draw(im);d.line(q,fill=color,width=2)
# dials precise reference centres / radii
face=Image.open(assets/'dial-face.png').resize((112,112),Image.Resampling.LANCZOS)
centres=[475,666,855,1048];colors=[(230,241,246),(0,217,250),(255,207,0),(244,0,235)];names=['THRESHOLD','WIDE','SPLIT','REPAIR'];vals=['−24.0 dB','55%','70%','40%'];props=[.67,.55,.70,.40]
for cx,c,name,val,p in zip(centres,colors,names,vals,props):
 d.text((cx,690),name,font=font(17),anchor='mm',fill=pale)
 im.alpha_composite(face,(cx-56,716));d=ImageDraw.Draw(im)
 box=(cx-62,710,cx+62,834);start=135;end=405
 glow=Image.new('RGBA',im.size);gd=ImageDraw.Draw(glow);gd.arc(box,start,start+(end-start)*p,fill=c+(150,),width=13);glow=glow.filter(ImageFilter.GaussianBlur(9));im=Image.alpha_composite(im,glow);d=ImageDraw.Draw(im);d.arc(box,start,end,fill=(54,72,83,220),width=2);d.arc(box,start,start+(end-start)*p,fill=c+(255,),width=4)
 ang=math.radians(start+(end-start)*p);x1=cx+math.cos(ang)*23;y1=772+math.sin(ang)*23;x2=cx+math.cos(ang)*42;y2=772+math.sin(ang)*42;d.line((x1,y1,x2,y2),fill=(250,252,252,255),width=4)
 d.text((cx,844),val,font=font(20),anchor='mm',fill=pale)
im.convert('RGB').save(R/'Design'/'ui-preview.png',quality=95)
