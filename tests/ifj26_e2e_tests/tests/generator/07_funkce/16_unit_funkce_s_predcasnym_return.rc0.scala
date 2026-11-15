import ifj26.*

def early(a: Int): Unit =
  if a < 0 then
    print("neg\n")
    return
  else
    print("pos\n")
  print("end\n")

@main def main(): Unit =
  val m = -1
  early(m)
  early(1)
