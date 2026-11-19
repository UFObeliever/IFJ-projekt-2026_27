import ifj26.*

@main
def main(): Unit =
  val c = false
  val d = if c then 1.5 else 2.5
  val s = toStr(d)
  print(s)
