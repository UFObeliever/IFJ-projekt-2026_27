import ifj26.*

extension (v: Int)
  def asDouble: Double = v

@main
def main(): Unit =
  val x = 3
  val r = x.asDouble
  if r == 3.0 then
    print("y")
  else
    print("n")
