import ifj26.*

@main
def main(): Unit =
  val d = 2.675
  val a = s"$d"
  val b = toStr(d)
  if a == b then
    print("same")
  else
    print("diff")
