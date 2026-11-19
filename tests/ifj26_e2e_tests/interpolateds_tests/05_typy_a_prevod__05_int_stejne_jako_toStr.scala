import ifj26.*

@main
def main(): Unit =
  val n = -42
  val a = s"$n"
  val b = toStr(n)
  if a == b then
    print("same")
  else
    print("diff")
