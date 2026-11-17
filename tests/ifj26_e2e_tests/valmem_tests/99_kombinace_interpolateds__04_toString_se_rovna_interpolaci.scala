import ifj26.*

@main
def main(): Unit =
  val d = 1.5
  val a = d.toString
  val b = s"$d"
  if a == b then
    print("same")
  else
    print("diff")
