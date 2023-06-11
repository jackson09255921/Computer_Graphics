# Computer_Graphics
## Abstract
渲染是一種將3D物體輸入計算機後進行光影繪製，著色等行為，最後將之呈現在2D屏幕當中的過程。對於玩家、研發工程師，甚至是一般僅有逛逛影音軟體需求的民眾來說，優良的渲染算法都是值得關注的議題。畢竟，4K畫質或是高FPS的項目，都是以渲染作為基底進行建構。  
關於此次專題開發，個人認為核心目標主要是能夠探索Mixed Reality的架構，因先前的專題已將CNN的動態捕捉項目進行研究產出對應結果。所以，本次專題將來研究另一大項目，計算機圖學。此項技術相對於機器視覺是將2D的圖片進行卷積運算，提取特徵來辨別3D物件，計算機圖學則是將3D模型經由一連串的線性轉換，最後降維到2D空間中進行呈現。  
從MR分析，他的架構是透過攝像頭與深度感測器來捕捉環境中的物體和人體肢體運動，這些數據透過機器視覺建立環境的三維模型，並將虛擬元素與現實世界結合。之後三維模型再藉由計算機圖學渲染給用戶，並使用光學原理與物體模擬來使其看起來更真實。  
因此此次開發鎖定光柵化(Rasterization)進行研究，從小畫家開始製作，循序漸進達成2D填充，實時渲染(Realtime Rendering Pipeline)，最終將著色(Shading)項目完成。



## Introduction
* 光柵化(Rasterization):是3D graph投影到2D平面的過程，其中可以再細分成管線化(Pipeline), 著色(Shading), 以及紋理(Texturing)。  
* 幾何(Geometry):包含曲線(Curve)與網格(Mesh)，顧名思義就是呈現幾何學，目前已知的相關內容有貝茲曲線(Bézier curve)以及陰影(Shadow)。  
* 光線追蹤(Ray tracing):藉由軟陰影(Soft shadow)、模糊反射(Glossy reflection)、間接光照(Indirection illumination)，來呈現比光柵化更精準、更真實的畫面，但相對輸出時間會慢上非常多，近年有研究出比Ray tracing更加擬真的渲染方式，稱為路徑追蹤(Path tracing)，當然所需要花費的cycle time又再度上升。  
* 動畫模擬(Animation):透過畫面呈現，立刻清空、並在平移一點距離的再次呈現。重複運作，達成視覺暫留成功欺騙眼和腦，使他們覺得看到了平滑運動的物體。這原理也被電視和電影廣泛應用。  
至於本次專題的目標-渲染是光線在環境中經由一系列彈跳，最後進入眼睛的過程。但在計算機領域當中，因為我們肉眼所見的畫面僅能透螢幕看見，因此在計算機圖學的定義為是一種將3D物體輸入計算機後進行光影繪製，著色等行為，最後將之呈現在2D屏幕當中。


## Method
這次專案開發項目僅涉及到光柵化中的管線化與著色，而動畫模擬的部分因礙於時間問題，課程尚未看到，因此先行留空。以下全部程式使用C++語言，畫螢幕著色部分僅調用openGL函式庫中「畫點功能」進行實作。最一開始，需要先學會如何在螢幕上畫出點、線、面，最後再延伸到立體空間。
![計算機圖學已知概念圖]()

## 參考資料:

[Lab1 Reference](https://hackmd.io/TpltIfbET3O9X64BYrsNKQ?view)

[Lab2 Reference](https://hackmd.io/5O2FIpo7RuCUEnjf0qvQiA?both)

[Lab3 Reference](https://hackmd.io/@leon890820/HJN9ahXNj)

[Lab4 Reference](https://hackmd.io/@leon890820/r1ceTQ5Dj)
