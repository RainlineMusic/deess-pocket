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
freq=np.geomspace(20,20000,420); log=np.log10(freq)
spec=np.interp(log,np.log10([20,50,100,200,400,1000,2000,5000,10000,20000]),
               [-55,-43,-28,-12,-20,-23,-27,-31,-38,-52])
rng=np.random.default_rng(4);spec+=rng.normal(0,2.5,len(freq))*np.clip((log-1.4),0,1)
y=np.clip(812-(spec+90)*650/90,145,862);pts=[(fx(f),v) for f,v in zip(freq,y)]
poly=pts+[(right,862),(left,862)]
mask=Image.new('L',im.size);ImageDraw.Draw(mask).polygon(poly,fill=255)
yy=np.arange(922)[:,None];a=np.clip(105*(1-yy/922)**1.6+8,8,110).astype('uint8')
alpha=np.repeat(a,1536,axis=1)
fill=Image.new('RGBA',im.size,(220,235,245,0))
fill.putalpha(Image.fromarray(np.minimum(np.asarray(mask),alpha)))
im=Image.alpha_composite(im,fill);d=ImageDraw.Draw(im)
glow=Image.new('RGBA',im.size);ImageDraw.Draw(glow).line(pts,fill=(239,248,255,180),width=5)
im=Image.alpha_composite(im,glow.filter(ImageFilter.GaussianBlur(6)));d=ImageDraw.Draw(im)
d.line(pts,fill=(245,251,255,230),width=2)
# Absolute repair threshold and one active signed response example.
threshold_y=812-(-52.2+90)*650/90
line=Image.new('RGBA',im.size);ImageDraw.Draw(line).line((left,threshold_y,right,threshold_y),fill=(255,24,237,140),width=2)
im=Image.alpha_composite(im,line.filter(ImageFilter.GaussianBlur(4)));d=ImageDraw.Draw(im)
d.line((left,threshold_y,right,threshold_y),fill=(255,24,237,140),width=1)
q=[]
for f in freq:
 red=12/(1+math.exp(-5*(math.log2(f/2500))))
 red+=4*math.exp(-((math.log(f/4200)/.15)**2))
 q.append((fx(f),162+red*9))
glow=Image.new('RGBA',im.size);ImageDraw.Draw(glow).line(q,fill=(255,24,237,130),width=10)
im=Image.alpha_composite(im,glow.filter(ImageFilter.GaussianBlur(9)));d=ImageDraw.Draw(im)
d.line(q,fill=(255,24,237,255),width=2)
# dials precise reference centres / radii
face=Image.open(assets/'dial-face.png').resize((112,112),Image.Resampling.LANCZOS)
centres=[475,666,855,1048];colors=[(230,241,246),(255,24,237),(255,24,237),(255,24,237)];names=['THRESHOLD','RATIO','LOW','SIBILANCE GAIN'];vals=['−52.2 dB','4.0:1','0.0 dB','0.0 dB'];props=[.42,.15,1,.5]
for cx,c,name,val,p in zip(centres,colors,names,vals,props):
 d.text((cx,690),name,font=font(17),anchor='mm',fill=pale)
 im.alpha_composite(face,(cx-56,716));d=ImageDraw.Draw(im)
 box=(cx-62,710,cx+62,834);start=135;end=405
 glow=Image.new('RGBA',im.size);gd=ImageDraw.Draw(glow);gd.arc(box,start,start+(end-start)*p,fill=c+(150,),width=13);glow=glow.filter(ImageFilter.GaussianBlur(9));im=Image.alpha_composite(im,glow);d=ImageDraw.Draw(im);d.arc(box,start,end,fill=(54,72,83,220),width=2);d.arc(box,start,start+(end-start)*p,fill=c+(255,),width=4)
 ang=math.radians(start+(end-start)*p);x1=cx+math.cos(ang)*23;y1=772+math.sin(ang)*23;x2=cx+math.cos(ang)*42;y2=772+math.sin(ang)*42;d.line((x1,y1,x2,y2),fill=(250,252,252,255),width=4)
 d.text((cx,844),val,font=font(20),anchor='mm',fill=pale)
im.convert('RGB').save(R/'Design'/'ui-preview.png',quality=95)
