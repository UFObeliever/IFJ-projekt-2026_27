import ifj26.*

@main
def main(): Unit =
  val n = 9
  val a = s"$n"
  val b = s"${n}"
  if a == b then
    print("same")
  else
    print("diff")
