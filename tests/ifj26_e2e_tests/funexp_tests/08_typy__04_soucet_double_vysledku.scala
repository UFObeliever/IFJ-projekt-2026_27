import ifj26.*

def hd(x: Double): Double = x / 2.0

@main
def main(): Unit =
  print(toStr(toInt(hd(3) + hd(5))))
