# Hardwaregrenze und Buderus-Ersatz

Die Leiterplatte existiert noch nicht. Dieser Stand programmiert keinerlei Pins oder elektrische Lüftersignale.

Bestätigt: Es sind **vier Lüfter** an der bestehenden VC40H angeschlossen. Die Modelle sind aktuell unbekannt. Für den Entwurf sind zwei gegensinnig arbeitende Paare vorgesehen; die tatsächliche Paarzuordnung muss vor Ort ermittelt werden.

Die [Buderus-Anleitung VC40 H, 6720875321 (2020/07)](https://www.heima24.de/shop/images/products/media/ia-ba-buderus-Bedieneinheit-VC40-H.pdf) beschreibt paarweise angeschlossene Logavent-Lüfter, vier Stufen, Richtungswechsel für Wärmerückgewinnung und einen Feuchtebetrieb. Der Anschlussplan nennt Versorgung, Masse und eine PWM-Leitung. Das ist keine ausreichende Spezifikation für einen ESP-GPIO-Treiber.

Vor dem Leiterplattenentwurf klären:

- Tatsächliche Lüftermodelle, Paarzuordnung der vier Lüfter und vorhandenes Netzteil.
- PWM-Spannung, Frequenz, Ausgangstopologie und Kodierung von Stufe und Richtung messen bzw. Herstellerdaten beschaffen.
- Verhalten bei Leitungsunterbrechung und fehlendem Steuersignal.
- Pegelanpassung, Schutzschaltung und definierter Ausgangszustand während ESP-Boot/Reset.
- Sensorik, Taster, Statusanzeige und Servicezugang.
- Falls relevant: bestehende externe Freigaben bzw. Drucküberwachung erhalten.
- Feuchteschutzkonzept und Betrieb bei Sensor- oder Netzwerkausfall festlegen.

Versorgungsspannung und PWM-Pegel dürfen nicht gleichgesetzt werden. Vor Messung und Schaltungsentwurf keine Lüfterleitungen direkt an den ESP anschließen.

Die reale Implementierung ersetzt später nur `FanOutput`. Die Anwendungsschnittstelle darf dafür um Richtung, Paarbetrieb und Fehlerzustände erweitert werden; eine elektrische Kodierung ist bewusst noch nicht festgelegt.
