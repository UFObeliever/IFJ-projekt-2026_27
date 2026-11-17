import ifj26.*

@main
def main(): Unit =
  val d = 2.7
  val m = s"${d.toInt}-${d.floor.toInt}"
  print(m)
