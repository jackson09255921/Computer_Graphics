# Computer_Graphics
Abstract
===
渲染是一種將3D物體輸入計算機後進行光影繪製，著色等行為，最後將之呈現在2D屏幕當中的過程。對於玩家、研發工程師，甚至是一般僅有逛逛影音軟體需求的民眾來說，優良的渲染算法都是值得關注的議題。畢竟，4K畫質或是高FPS的項目，都是以渲染作為基底進行建構。  
關於此次專題開發，個人認為核心目標主要是能夠探索Mixed Reality的架構，因先前的專題已將CNN的動態捕捉項目進行研究產出對應結果。所以，本次專題將來研究另一大項目，計算機圖學。此項技術相對於機器視覺是將2D的圖片進行卷積運算，提取特徵來辨別3D物件，計算機圖學則是將3D模型經由一連串的線性轉換，最後降維到2D空間中進行呈現。  
從MR分析，他的架構是透過攝像頭與深度感測器來捕捉環境中的物體和人體肢體運動，這些數據透過機器視覺建立環境的三維模型，並將虛擬元素與現實世界結合。之後三維模型再藉由計算機圖學渲染給用戶，並使用光學原理與物體模擬來使其看起來更真實。  
因此此次開發鎖定光柵化(Rasterization)進行研究，從小畫家開始製作，循序漸進達成2D填充，實時渲染(Realtime Rendering Pipeline)，最終將著色(Shading)項目完成。



Introduction
===
* 光柵化(Rasterization):是3D graph投影到2D平面的過程，其中可以再細分成管線化(Pipeline), 著色(Shading), 以及紋理(Texturing)。  
* 幾何(Geometry):包含曲線(Curve)與網格(Mesh)，顧名思義就是呈現幾何學，目前已知的相關內容有貝茲曲線(Bézier curve)以及陰影(Shadow)。  
* 光線追蹤(Ray tracing):藉由軟陰影(Soft shadow)、模糊反射(Glossy reflection)、間接光照(Indirection illumination)，來呈現比光柵化更精準、更真實的畫面，但相對輸出時間會慢上非常多，近年有研究出比Ray tracing更加擬真的渲染方式，稱為路徑追蹤(Path tracing)，當然所需要花費的cycle time又再度上升。  
* 動畫模擬(Animation):透過畫面呈現，立刻清空、並在平移一點距離的再次呈現。重複運作，達成視覺暫留成功欺騙眼和腦，使他們覺得看到了平滑運動的物體。這原理也被電視和電影廣泛應用。  
至於本次專題的目標-渲染是光線在環境中經由一系列彈跳，最後進入眼睛的過程。但在計算機領域當中，因為我們肉眼所見的畫面僅能透螢幕看見，因此在計算機圖學的定義為是一種將3D物體輸入計算機後進行光影繪製，著色等行為，最後將之呈現在2D屏幕當中。 




Method
===
這次專案開發項目僅涉及到光柵化中的管線化與著色，而動畫模擬的部分因礙於時間問題，課程尚未看到，因此先行留空。以下全部程式使用C++語言，畫螢幕著色部分僅調用openGL函式庫中「畫點功能」進行實作。最一開始，需要先學會如何在螢幕上畫出點、線、面，最後再延伸到立體空間。  

![計算機圖學已知概念圖](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image25.png)

---
## Lab 1 小畫家
小畫家環節呈現出按住滑鼠畫連續點、畫直線、畫多邊形、畫圓形、清空、回上一步以及換色的功能。點的部分，因為不含任何運算技術，就直接調用glut函式中的畫點功能。
1. 畫線: 決定畫面上的兩個位置，並將鄰近兩點連線中的pixel上色，透過iteration的方式讓vertex從起點朝向終點逐一檢查，每當vertex到pixel的邊界時，使用線性插值檢查相鄰pixel的距離，選取相對接近的點進行著色後進續迭代直到終點。這個方法需要每45度建構一個，因此建構共8個方向，並透過斜率與起終點方向來決定應當使用的演算法。
2. 畫多邊形: 概念如同畫直線，唯一差別是要另外構建繪製起終點連線的觸發機制，在此不多做贅述。
3. 畫圓形: 選取圓心與任意點，並以圓心與該點距離作為半徑繪製圓形。演算法概念與畫直線雷同，使用iteration的方式減少運算，並切割成8個部分進行運算，下面取北北東方向進行說明。首先，我們透過半徑已知正北方與東北方兩個點的位置，接下來給予一個vertex從正北方iterate到東北方進行迭代，出發點往右偏下移動，與pixel交界相交時進行插值選取最近點著色。再來，因方向需沿切線方向移動(畫直線只需朝終點直線移動)，因此需要根據上一個著色的點來做為方向準則。透過這個方式可以繪製出1/8個圓弧，然後八個方向各做一次即可。
4. 清除: 如同畫點不需技術，這裡使用glut的更新畫布功能
5. 回上一步:將各圖形事先用Class封裝點座標與形狀類型，並另外建一個stack進行儲存。如此一來，就可以根據情況選擇要將stack的element取出來對位置進行塗黑，或是上色，達成返回的功能。
6. 換色:一樣沒技術可言，使用glut函式與亂數切換顏色。  
#### Lab1 成果
![小畫家](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image28.png)

---
## Lab2 2D填充
上一個環節，我們成功勾勒出圖形外框，接下來我們要把整個面構建起來。這邊要做四件事情，外部讀檔、圖形填充、viewport以及圖形切割。
1. 外部讀檔: 使用```main(int argc, char* argv[])```進行外部檔案讀取即可。用意在於最終目標為讀入.obj模型座標與設定各項參數條件進行繪製，不存在人工繪製的需求，所以我們建立輸入參數的管道。
2. 圖形填充: 使用外積的方式進行檢測目前的pixel是否在圖形當中，若是則上色，反之直接跳過。
3. Viewport transform(又稱為視圖變換):一個視圖在螢幕中可能在右上方、左下方、正中方等不同位置，就如同開啟了一個瀏覽器，但覺得太佔空間，因此把他縮小並放到旁邊，讓螢幕可以多呈現一個像是文書軟體的其他畫面。計算方式為將圖形(座標點)依序作平移回原點、旋轉、平移至目標點的仿射變換。
4. 圖形切割( 2D clipping):先決定上下左右的切割順序，接著根據當前位置決定內外邊(Inside/Outside)，根據邊界切割演算法將超出視圖的頂點進行清除、保留或增加，達成邊界切割，演算法如下。  

![邊界切割演算法](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image30.png)
#### Lab2 成果
![2D填充-1](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image35.png ) ![2D填充-2](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image33.png)

---
## Lab3 3D Pipeline
第三小節，將介紹3D Pipeline。我們在前兩小節已經學會如何在螢幕上呈現常見的幾何圖形了，那接下來就是要把3D object丟到螢幕上方，並且可以「立即」看到他的成像。這部分的演算法一共有6個(4個空間轉換 + 3D clipping + Hide Surface Removal)，下面依序介紹。

![3D Pipeline](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image40.png)
1. Model transformation:當一個物體被輸入後可以藉由模型變換轉到世界座標，涵蓋的變換有縮放、旋轉、平移(三者合稱仿射變換)，其中平移變換並非為線性變換，因此引入齊次座標系統。  

![齊次座標系統](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image36.png)

2. Viewing transformation:這項轉換本質上是為了將攝影機的視線轉到+z軸，將上方調整為+y軸，但因為比起調整視野，不如調整物體在空間中的相對位置，同樣可以達成相同的效果。根據上面所述，模型變換和視野變換都是對物體進行線性轉換，因此常用ModelView Transformation進行稱呼。  

![Model View Transformation](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image38.png)

3. Projection transformation:此一環節正式將3D物體投影到2D畫面，做法為固定Near clipping plane與Far clipping plane，將x軸y軸的座標透過相似三角形原理進行線性轉換，進入Projection Space。Projection space再進行perspective divide，簡單講就是將(x, y, z, w)的改成(x/w, y/w, z/w, 1)，最終圖形的點座標會被壓縮在[-1,1]*[-1,1]*[0,1]的範圍中，進入Image space。  

![Projection Transformation](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image41.png)

4. Viewport Transform:作用與第二小節的viewport相同，只是用3D的仿射轉換進行處理，另外已需要將image space的視窗大小變換到螢幕大小。如此一來整個pipeline基礎運算就介紹完畢。  

![3D pipeline](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image45.png)

5. 3D clipping:這邊是進階應用，當想要讓圖在某個特定區域出現時可以使用，處理方式如同2D clipping。在Image space實作，差別是需要在六個面(left, right , top, bottom, near, far)都作切割一次。  

![3D clipping](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image47.png)

6. Hide Surface Removal:這一小節的最後一項，作用是去除背向攝影機的三角形，在World Space實作。透過三角形的法向量與相機的視線進行內積，若小於0(夾角超過90度)則視為背向攝影機，直接刪除。  

<img src="https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image48.jpg" width="375">
<img src="https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image49.jpg" width="375">


---
## Lab4 3D Shading
著色(Shading)系統是透過近似的方式將光照模擬出來，透過此方式可以藉由犧牲準確度，換取及時光照渲染，以下採用馮布林光照模型(Blinn-Phong model)進行建構。Blinn-Phong模型將光照切割成三種不同光照模式，最後再透過疊加原理模擬出假光照，分為環境光(Ambient)、漫反射(Diffusion)、高光(Specular)，光照計算World space進行。

![Blinn-Phong Shading Model](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image50.png)

1. 環境光(Ambient): 模擬人眼在夜晚無直接光照下看到的情形，因周遭仍有細微光源彈射(例如:月亮)。當我們在夜晚進行觀察的時候，若前方有物體，可以隱約見到物體的形狀，但無法辨識他的遠近，以及真實外觀(圖17)。環境光不考慮光源方向以及視線方向，每個位置成效均一。  
- 概念圖:   
![Ambient 說明](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image51.png)  
- Ambient 成果:  
![Ambient 成果](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image15.png)

2. 漫反射(Diffusion): 模擬物體受到直接光照後光線彈射到眼睛的過程，像是我們在讀書時可能會開燈，光線會先彈射到書面上，之後再進入瞳孔，成為我們看到燈光照在書上的畫面。
- 概念圖:  

![Diffusion 說明](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image16.png)
- 光強度衰減率:  

![光強度衰減率](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image17.png)  
- Diffusion 示意圖:  

![漫反射示意圖](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image18.png)  
漫反射在乎光源方向與平面法向量形成的夾角，當夾角越小，代表單位面積接收到的光強度越大(圖18)。另一方面，光傳遞的距離也與強度有關(強度與距離平方成反比)。下方為漫反射公式，其中r為光源與著色點的距離，kd為漫反射係數，Lm為著色點向光源的單位向量，N為著色點的單位(虛擬)法向量，Summation代表多點光源照射。  

![漫反射](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image19.png)

3. 高光(Specular):模擬物體被直接光照後，人眼正好在反射線附近所看到的畫面。通常這個區域僅在反射線夾角正負5度的距離才有機會見到，如玻璃盤上方最亮的部分。  
- 玻璃盤:  

![玻璃盤](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image20.jpg)  
- Specular 示意圖:  

![高光示意圖](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image21.png)  
- Specular 成果:  

![Specular 成果](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image22.png)  

最後，將三種光照方式進行線性組合可以得到圖26的各個畫面，最右邊為疊加的最終成果。以上最基礎的光照方式，下一part將呈現三種不同的著色頻率方式以及本次專題的最終成果(Flat Shading/Gouraud Shading/Phong Shading)。
![Blinn-Phong光照成果](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image23.png)

---
三種著色方式(Flat Shading、Gouraud Shading、Phong Shading)
===
1. 著色頻率
著色頻率(Shading Frequencies)，依照虛擬法向量以及光照的關係達成三種截然不同的畫面。  
- Shade each triangle(Flat Shading):以面為單位進行法向量計算，換言之就是整個面上任何區域的虛擬法向量都一樣，之後再進行光照。達成效果為單一面為同一顏色，不存在連續漸變。
- Shade each vertex(Gouraud Shading):以點為單位進行法向量計算，其(虛擬)法向量根據該頂點周遭三角形的法向量進行加權平均求得，之後對每個頂點進行光照，最後將光照插值分配給每個像素。  

![點的虛擬法向量計算](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image24.png)  
- Shade each pixel(Phong Shading):以pixel為單位進行法向量計算，其計算方式將代入重心座標進行處理(也可以丟到雙線性插值來計算，但程式要處理一大堆的例外狀況，因此不採納)，求得後對於每一個pixel進行光照，最後就可以獲得Phong Shading。   

![Pixel的法向量計算](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image32.png)  

2. Z-buffer
先前提到的所有演算法，都只有涉及到「單一3D物體」投影到2D螢幕上的過程，那如果需要同時將「多個3D物體」渲染在螢幕中，該如何避免圖形在2D平面上重疊的問題，這個就是Z-buffer將解決的部分。  
Z-buffer稱為深度緩衝，基於螢幕所見到的畫面以x-y軸進行投影，z軸扮演衡量物體前後相對距離的角色，這個相對關係稱為深度，機器視覺所提到的深度相機也代有此含意。如圖29，他的作法是在Image space中對於物體每面，面被pixel所涵蓋的區域進行z值查驗，若其z值為越小，代表離螢幕越近，Z-buffer就會儲存它的資訊來做後續比較，同時間也會用Color buffer(frame buffer)來蒐集目前區域的顏色。反之，保留背景顏色。根據此規則掃蕩每個物體中的每一塊三角形區域。最後用Color buffer內的顏色值投影到螢幕當中。  

![Z-buffer成效](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image37.png)  
到了這裡，已經是整個pipeline與shading最後環節，其他的像是程式該如何編寫，obj檔案的格式與象徵意義，以及Rasterization的最後一個環節-Texturing部分歡迎到計算機圖學課程或是games101課程進行深究。


---
Conclusion & Demo
===
以下為由左至右依序為Flat Shading、Gouraud Shading以及Phong Shading的渲染作品呈現，當離物體非常近的時候，單位三角形需要大量的pixel進行涵蓋，因此我們可以很明顯看出三種算法之間的差異性。但當把距離拉遠，變成單位pixel內部蘊含大量的三角形，那渲染效果就不會有顯著感受。未來後方向可以把計算機圖學的其他單元全部實現一遍，就算是完成Mixed Reality中的第二大關。

![最終成果1](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image39.png)  

![最終成果2](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image42.png)  

![最終成果3](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image44.png)  

![最終成果3](https://github.com/jackson09255921/Computer_Graphics/blob/main/images/image46.png)  

---

Reference
===
- [Lab1 Reference](https://hackmd.io/TpltIfbET3O9X64BYrsNKQ?view)
- [Lab2 Reference](https://hackmd.io/5O2FIpo7RuCUEnjf0qvQiA?both)
- [Lab3 Reference](https://hackmd.io/@leon890820/HJN9ahXNj)
- [Lab4 Reference](https://hackmd.io/@leon890820/r1ceTQ5Dj)
- [GAMES 101](https://www.bilibili.com/video/BV1X7411F744/?vd_source=079f4f5683be9efc4fbd04ff20338398)
