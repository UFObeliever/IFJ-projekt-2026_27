import ifj26.*

@main
def main(): Unit =
  val x = 2.5
  val l = (x * 100.0).floor.toInt.toString.length
  print(l)
