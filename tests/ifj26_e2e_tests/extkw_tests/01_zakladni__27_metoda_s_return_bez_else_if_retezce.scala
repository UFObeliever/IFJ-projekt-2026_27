import ifj26.*

extension (v: Int)
  def kind: String =
    if v < 0 then
      return "neg"
    else if v == 0 then
      return "zero"
    else
      return "pos"

@main
def main(): Unit =
  val a = 0 - 3
  val b = 0
  val c = 9
  val r1 = a.kind
  val r2 = b.kind
  val r3 = c.kind
  print(r1)
  print(r2)
  print(r3)
