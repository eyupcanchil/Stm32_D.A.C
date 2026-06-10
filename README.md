STM32 Black Pill ile Harici R-2R Ladder DAC Tasarımı
Proje Hakkında

Bu projede amacımız, dijital verilerin analog sinyallere dönüştürülme sürecini yalnızca teorik olarak değil, doğrudan donanım seviyesinde deneyimlemekti. STM32 mikrodenetleyicilerinde dahili DAC birimleri bulunsa da, sistemin nasıl çalıştığını daha iyi anlayabilmek ve dijital-analog dönüşümün temel mantığını kavrayabilmek adına harici bir R-2R Direnç Merdiveni (R-2R Ladder DAC) tasarladık.

Kontrolcü olarak, sunduğu GPIO esnekliği, performansı ve gömülü sistem projelerine uygunluğu nedeniyle STM32 Black Pill geliştirme kartını tercih ettik.

Proje Hedefleri
R-2R Ladder yapısının çalışma mantığını uygulamalı olarak öğrenmek
Mikrodenetleyici GPIO çıkışlarını doğrudan kullanarak DAC gerçekleştirmek
Dijital girişlerden analog çıkış üretmek
Simülasyon ve gerçek donanım sonuçlarını karşılaştırmak
Gömülü sistemlerde donanım-yazılım entegrasyonunu deneyimlemek
Kullanılan Bileşenler
Donanım
STM32 Black Pill (STM32F401CCU6)
4x4 Matris Keypad
R ve 2R değerlerinden oluşan direnç ağı
7-Segment Gösterge
Breadboard
Jumper kablolar
Güç kaynağı
Yazılım
STM32CubeIDE
Proteus Design Suite
Sistem Mimarisi

Sistem üç temel bölümden oluşmaktadır:

Keypad Girişi
Kullanıcı tarafından girilen sayısal değerler okunur.
STM32 Black Pill
Keypad'den gelen veriyi işler.
İlgili bit desenini GPIO pinlerine aktarır.
R-2R Ladder DAC
GPIO pinlerinden gelen dijital bitleri analog gerilime dönüştürür.
Çıkışta teorik olarak doğrusal bir analog seviye elde edilir.

Ayrıca kullanıcı tarafından girilen değerlerin takip edilebilmesi amacıyla sistemde 7-segment gösterge kullanılmıştır.

Simülasyon Süreci

Fiziksel kuruluma geçmeden önce olası bağlantı ve tasarım hatalarını minimuma indirebilmek amacıyla sistemin tamamı Proteus ortamında modellenmiştir.

Simülasyon devresi aşağıdaki bileşenlerden oluşturulmuştur:

STM32 Black Pill
4x4 Keypad
R-2R Direnç Ağı
7-Segment Gösterge

Yapılan testlerde farklı dijital girişlere karşılık analog çıkışın beklenen şekilde değiştiği gözlemlenmiş ve sistemin kararlı çalıştığı doğrulanmıştır.

Donanım Kurulumu

Simülasyon sonuçlarının başarılı olması üzerine devrenin fiziksel kurulumu gerçekleştirilmiştir.

Breadboard üzerine hassas şekilde yerleştirilen direnç ağı ile birlikte:

STM32 Black Pill bağlantıları yapıldı.
Keypad girişleri bağlandı.
7-segment gösterge entegre edildi.
R-2R Ladder çıkışı ölçüm cihazları ile doğrulandı.

Gerçek donanım testlerinde elde edilen sonuçlar simülasyon verileriyle büyük ölçüde örtüşmüş ve doğrusal analog çıkış davranışı başarıyla gözlemlenmiştir.
