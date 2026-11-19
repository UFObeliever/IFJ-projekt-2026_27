import ifj26.*

@main
def main(): Unit =
  val d = 123.9
  val m = s"${d.floor.toInt.toString.length}"
  print(m)
